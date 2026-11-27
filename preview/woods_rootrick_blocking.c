/* SPDX-License-Identifier: MIT */
#include "woods_rootrick_blocking.h"
#include "village_talk_camera.h"
#include "field_waypoint.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>

struct BfmWoodsRootrickBlocking {
    const uint8_t *scene;
    size_t bytes;
    BfmBedroomTravel *travel;
    BfmVillageTalkCamera shot;
    const uint8_t *race_sine;
    int race_y, race_pitch;
    unsigned return_fields;
    BfmVillageCamera returning;
};

static const uint32_t scene_base = UINT32_C(0x80128158);
static const uint32_t room_header = UINT32_C(0x801961e8);

BfmWoodsRootrickBlocking *bfm_woods_rootrick_blocking_open(const uint8_t *scene, size_t bytes,
                                                          const BfmBedroomRoot *player)
{
    if (!scene || !player) return NULL;
    BfmWoodsRootrickBlocking *blocking = calloc(1, sizeof *blocking);
    if (!blocking) return NULL;
    blocking->travel = bfm_bedroom_travel_from_root(scene, bytes, room_header, player);
    if (!blocking->travel) {
        free(blocking);
        return NULL;
    }
    blocking->scene = scene;
    blocking->bytes = bytes;
    return blocking;
}

void bfm_woods_rootrick_blocking_free(BfmWoodsRootrickBlocking *blocking)
{
    if (!blocking) return;
    bfm_bedroom_travel_free(blocking->travel);
    free(blocking);
}

int bfm_woods_rootrick_walk(BfmWoodsRootrickBlocking *blocking, int16_t x, int16_t z)
{
    return blocking && bfm_bedroom_travel_walk(blocking->travel, x, z, 0);
}

int bfm_woods_rootrick_face(BfmWoodsRootrickBlocking *blocking, int16_t x, int16_t z)
{
    if (!blocking) return 0;
    const BfmBedroomRoot *player = bfm_bedroom_travel_root(blocking->travel);
    if (player->busy) return 0;
    /* An actor look samples its target once. Let the ordinary player turn
     * service ease the model heading and expose completion on root.busy. */
    BfmBedroomTravel *travel = blocking->travel;
    int dx = player->xyz[0] - x, dz = player->xyz[2] - z;
    unsigned goal = bfm_waypoint_heading(dx, dz);
    return bfm_bedroom_travel_turn(travel, goal, 0);
}

int bfm_woods_rootrick_step_player(BfmWoodsRootrickBlocking *blocking,
                                   const BfmVillageCamera *previous_view)
{
    return blocking && previous_view &&
           bfm_bedroom_travel_step(blocking->travel, previous_view->eye, previous_view->target);
}

const BfmBedroomRoot *bfm_woods_rootrick_player(const BfmWoodsRootrickBlocking *blocking)
{
    return blocking ? bfm_bedroom_travel_root(blocking->travel) : NULL;
}

const int64_t *bfm_woods_rootrick_player_position(const BfmWoodsRootrickBlocking *blocking)
{
    return blocking ? bfm_bedroom_travel_position(blocking->travel) : NULL;
}

int bfm_woods_rootrick_frame(BfmWoodsRootrickBlocking *blocking, uint32_t record_address,
                             const BfmVillageCamera *presented)
{
    if (!blocking || !presented || record_address < scene_base || blocking->bytes < 16)
        return 0;
    size_t offset = record_address - scene_base;
    if (offset > blocking->bytes - 16) return 0;
    const uint8_t *record = blocking->scene + offset;
    int eye[3], target[3];
    for (unsigned i = 0; i < 3; ++i) {
        target[i] = s16le(record + 2*i);
        eye[i] = s16le(record + 8 + 2*i);
    }
    blocking->race_sine = NULL;
    blocking->return_fields = 0;
    return bfm_village_talk_camera_world(&blocking->shot, eye, target, presented, presented);
}

void bfm_woods_rootrick_step_camera(BfmWoodsRootrickBlocking *blocking, BfmVillageCamera *view)
{
    if (blocking) bfm_village_talk_camera_step(&blocking->shot, view);
}

void bfm_woods_rootrick_release_camera(BfmWoodsRootrickBlocking *blocking)
{
    if (blocking) {
        blocking->shot = (BfmVillageTalkCamera){0};
        blocking->race_sine = NULL;
        blocking->return_fields = 0;
    }
}

static int sine15(const uint8_t *quarter_wave, unsigned angle)
{
    unsigned sample = angle & 1023u;
    if (angle & 1024u) sample = 1023u - sample;
    int amplitude = u16le(quarter_wave + 2*sample);
    return angle & 2048u ? -amplitude : amplitude;
}

static int scale15(int a, int b)
{
    int product = a*b;
    return product / 32768 - (product % 32768 < 0);
}

int bfm_woods_rootrick_race_camera_begin(BfmWoodsRootrickBlocking *blocking,
                                         const int16_t foot[3], BfmVillageCamera *view)
{
    size_t offset = UINT32_C(0x801d66c8) - scene_base;
    if (!blocking || !foot || !view || offset > blocking->bytes || blocking->bytes - offset < 2048)
        return 0;
    bfm_woods_rootrick_release_camera(blocking);
    blocking->race_sine = blocking->scene + offset;
    blocking->race_y = foot[1];
    blocking->race_pitch = 113;
    return bfm_woods_rootrick_race_camera_step(blocking, foot, view);
}

int bfm_woods_rootrick_race_camera_step(BfmWoodsRootrickBlocking *blocking,
                                        const int16_t foot[3], BfmVillageCamera *view)
{
    if (!blocking || !blocking->race_sine || !foot || !view) return 0;
    int distance_y = foot[1] - blocking->race_y;
    int step_y = distance_y / 4;
    if (!step_y && distance_y) step_y = distance_y < 0 ? -1 : 1;
    blocking->race_y += step_y;
    /* Both elevation bands tilt the orbit as Musashi climbs. Between them,
     * and above the upper band, the camera keeps its last inclination. */
    if (blocking->race_y >= -639) blocking->race_pitch = blocking->race_y + 753;
    else if (blocking->race_y >= -1151 && blocking->race_y <= -896)
        blocking->race_pitch = blocking->race_y + 1009;

    unsigned azimuth = (unsigned)(bfm_waypoint_heading(foot[0], foot[2]) - 341);
    unsigned pitch = (unsigned)blocking->race_pitch;
    const uint8_t *wave = blocking->race_sine;
    int horizontal = sine15(wave, pitch + 1024u);
    int level = sine15(wave, 1024u);
    int x = scale15(scale15(horizontal, level), sine15(wave, azimuth));
    int y = -scale15(sine15(wave, pitch), level);
    int z = scale15(horizontal, sine15(wave, azimuth + 1024u));

    view->focal = 500;
    view->radius = 1200;
    view->target[0] = foot[0] + 224;
    view->target[1] = blocking->race_y;
    view->target[2] = foot[2];
    view->eye[0] = view->target[0] + scale15(x, view->radius);
    view->eye[1] = view->target[1] + scale15(y, view->radius);
    view->eye[2] = view->target[2] + scale15(z, view->radius);
    view->yaw = bfm_waypoint_heading(view->eye[0] - view->target[0], view->eye[2] - view->target[2]);
    return 1;
}

void bfm_woods_rootrick_room_camera_begin(BfmWoodsRootrickBlocking *blocking,
                                         const BfmVillageCamera *previous)
{
    if (!blocking || !previous) return;
    bfm_woods_rootrick_release_camera(blocking);
    blocking->returning = *previous;
    blocking->return_fields = 48;
}

static int camera_half_step(int from, int to)
{
    int difference = to - from;
    if (difference == 1 || difference == -1) return to;
    return from + difference / 2;
}

int bfm_woods_rootrick_room_camera_step(BfmWoodsRootrickBlocking *blocking,
                                       BfmVillageCamera *room)
{
    if (!blocking || !room || !blocking->return_fields) return 0;
    BfmVillageCamera *saved = &blocking->returning;
    for (unsigned i = 0; i < 3; ++i) {
        saved->eye[i] = camera_half_step(saved->eye[i], room->eye[i]);
        saved->target[i] = camera_half_step(saved->target[i], room->target[i]);
        room->eye[i] = saved->eye[i];
        room->target[i] = saved->target[i];
    }
    --blocking->return_fields;
    return 1;
}
