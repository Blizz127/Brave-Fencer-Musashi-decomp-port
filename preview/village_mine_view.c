/* SPDX-License-Identifier: MIT */
#include "village_mine_view.h"
#include "field_waypoint.h"
#include "forest_lift.h"
#include "scene_tiles.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>

enum { ZONES = 9, ZONE_BYTES = 16 };

struct BfmVillageMineView {
    BfmVillageField field;
    BfmVillageCamera camera;
    uint16_t (*tiles)[1024];
    const uint8_t *zones;
    int goal_pitch, goal_yaw, goal_radius;
    int pitch, yaw, radius, anchor[3];
};

BfmVillageMineView *bfm_village_mine_view_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, uint16_t tiles[512][1024])
{
    const uint32_t base = UINT32_C(0x80128158), room = UINT32_C(0x8018615c);
    const size_t binding = UINT32_C(0x8018245c) - base;
    const size_t zones = UINT32_C(0x80185ff0) - base;
    if (!scene || !disc || !tiles || bytes < room - base + 0xcc ||
        u32le(scene + binding) != room) return NULL;
    for (unsigned i = 0; i < ZONES; ++i) {
        const uint8_t *zone = scene + zones + ZONE_BYTES * i;
        if (s16le(zone) > s16le(zone + 2) || s16le(zone + 4) > s16le(zone + 6) ||
            u16le(zone + 8) >= 4096 || u16le(zone + 10) >= 4096 ||
            !u32le(zone + 12) || u32le(zone + 12) > INT16_MAX) return NULL;
    }
    BfmVillageMineView *view = calloc(1, sizeof *view);
    if (!view) return NULL;
    if (!bfm_village_field_load_room(&view->field, disc, scene, bytes, room, UINT32_C(0x801842d4)) ||
        !bfm_scene_read_tiles(disc, "MAIN.CD", 10, tiles) ||
        !bfm_scene_read_tiles(disc, "SC03.CD", 107, tiles)) {
        bfm_village_mine_view_free(view);
        return NULL;
    }
    view->zones = scene + zones;
    view->tiles = tiles;
    return view;
}

void bfm_village_mine_view_free(BfmVillageMineView *view)
{
    if (view) {
        bfm_village_field_clear(&view->field);
        free(view);
    }
}

BfmVillageField *bfm_village_mine_view_field(BfmVillageMineView *view)
{
    return view ? &view->field : NULL;
}

const BfmVillageCamera *bfm_village_mine_view_camera(const BfmVillageMineView *view)
{
    return view ? &view->camera : NULL;
}

int bfm_village_mine_view_pad_yaw(const BfmVillageMineView *view)
{
    if (!view) return 0;
    const BfmVillageCamera *c = &view->camera;
    return bfm_waypoint_heading(c->target[0] - c->eye[0], c->target[2] - c->eye[2]);
}

static int mine_zone(BfmVillageMineView *view, int target[3])
{
    int lower = target[2] > 256 ? -1856 : -1600;
    int upper = target[2] > 256 ? 2624 : 1408;
    if (target[0] < lower) target[0] = lower;
    if (target[0] > upper) target[0] = upper;
    for (unsigned i = 0; i < ZONES; ++i) {
        const uint8_t *zone = view->zones + ZONE_BYTES * i;
        int left = s16le(zone), right = s16le(zone + 2);
        int min_z = s16le(zone + 4), max_z = s16le(zone + 6);
        if (target[0] < left || target[0] > right || target[2] < min_z || target[2] > max_z)
            continue;
        view->goal_pitch = u16le(zone + 8);
        view->goal_yaw = u16le(zone + 10);
        view->goal_radius = (int)u32le(zone + 12);
        /* Follow the corridor centerline. The junctions follow both axes. */
        if (i == 2 || i == 6) target[0] = left + (right - left) / 2;
        if (i == 0 || i == 4 || i == 5 || i == 8) target[2] = min_z + (max_z - min_z) / 2;
        return 1;
    }
    return 0;
}

static int track_delta(int delta, int divisor, int taper)
{
    if (taper)
        while (divisor > 1 && delta / divisor == 0) divisor /= 2;
    int step = delta / divisor;
    return step || !delta ? step : (delta < 0 ? -1 : 1);
}

static int track_angle(int angle, int goal, int in_zone)
{
    int delta = goal - angle;
    if (delta > 2048) delta -= 4096;
    if (delta < -2048) delta += 4096;
    return (angle + track_delta(delta, 10, !in_zone)) & 4095;
}

void bfm_village_mine_view_follow(BfmVillageMineView *view, const int16_t foot[3], int entering)
{
    if (!view || !foot) return;
    BfmVillageCamera *camera = &view->camera;
    int target[3] = {foot[0], foot[1] - 50, foot[2]};
    if (entering) {
        view->goal_pitch = view->goal_yaw = 0;
        view->pitch = 113;
        view->yaw = 3072;
        view->radius = view->goal_radius = 600;
        camera->focal = 400;
        camera->following = 1;
        for (unsigned i = 0; i < 3; ++i) view->anchor[i] = target[i];
    }
    int in_zone = mine_zone(view, target);
    view->pitch = track_angle(view->pitch, view->goal_pitch, in_zone);
    view->yaw = track_angle(view->yaw, view->goal_yaw, in_zone);
    view->radius += track_delta(view->goal_radius - view->radius, 4, 1);
    camera->yaw = view->yaw;
    camera->radius = view->radius;
    int16_t anchor[3];
    for (unsigned i = 0; i < 3; ++i) {
        view->anchor[i] += track_delta(target[i] - view->anchor[i], 4, 0);
        camera->target[i] = view->anchor[i];
        anchor[i] = (int16_t)view->anchor[i];
    }
    BfmForestTransform orbit;
    int16_t arm[3] = {0, 0, (int16_t)camera->radius}, eye[3];
    bfm_forest_actor_pitch_yaw((unsigned)view->pitch, (unsigned)view->yaw, anchor, &orbit);
    bfm_forest_lift_world(&orbit, arm, eye);
    for (unsigned i = 0; i < 3; ++i) camera->eye[i] = eye[i];

    /* Entry publishes one view from the pad before centering the tracking
     * state used by the first ordinary update. */
    if (entering) {
        view->pitch = view->goal_pitch;
        view->yaw = view->goal_yaw;
        view->radius = view->goal_radius;
        for (unsigned i = 0; i < 3; ++i) view->anchor[i] = target[i];
    }
}

int bfm_village_mine_view_draw(BfmVillageMineView *view, const BfmBedroomRoot *root,
    const BfmBedroomPose *pose, const uint8_t flags[64], unsigned age, unsigned scale_q12,
    uint32_t pixels[240][320],
    int (*submit)(void *, BfmBedroomFigure *, const BfmBedroomView *), void *actor)
{
    if (!view || !root || !pose || !flags || !pixels) return 0;
    for (unsigned y = 0; y < 240; ++y)
        for (unsigned x = 0; x < 320; ++x) pixels[y][x] = UINT32_C(0xff000000);
    if (!bfm_village_field_prepare_scaled(&view->field, &view->camera, root, pose, flags,
        age, bfm_village_field_visible(pose, flags), scale_q12, view->tiles, &pixels[0][0])) return 0;
    const BfmBedroomView *projection = bfm_bedroom_field_view(view->field.field);
    if (submit && !submit(actor, view->field.hero, projection)) return 0;
    bfm_bedroom_field_draw(view->field.field, view->tiles, &pixels[0][0],
                           bfm_bedroom_figure_draw_to, view->field.hero);
    return 1;
}
