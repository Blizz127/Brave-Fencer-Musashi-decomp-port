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
    const uint8_t *zones, *bearings, *wave, *distances;
    unsigned zoom;
    int goal_pitch, goal_yaw, goal_radius;
    int pitch, yaw, radius, anchor[3];
};

BfmVillageMineView *bfm_village_mine_view_open_field(const MusashiDiscMedia *disc, const uint8_t *scene,
    size_t bytes, uint32_t room, uint32_t face_table, unsigned member, uint16_t tiles[512][1024])
{
    if (!disc || !scene || !tiles) return NULL;
    BfmVillageMineView *view = calloc(1, sizeof *view);
    if (!view) return NULL;
    if (!bfm_village_field_load_room(&view->field, disc, scene, bytes, room, face_table) ||
        !bfm_scene_read_tiles(disc, "MAIN.CD", 10, tiles) ||
        !bfm_scene_read_tiles(disc, "SC03.CD", member, tiles)) {
        bfm_village_mine_view_free(view);
        return NULL;
    }
    view->tiles = tiles;
    return view;
}

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
    BfmVillageMineView *view = bfm_village_mine_view_open_field(disc, scene, bytes, room, UINT32_C(0x801842d4), 107, tiles);
    if (view) view->zones = scene + zones;
    return view;
}

BfmVillageMineView *bfm_village_mine_corridor_view_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, uint16_t tiles[512][1024])
{
    const uint32_t base = UINT32_C(0x80128158), room = UINT32_C(0x80184870);
    const size_t binding = UINT32_C(0x80180c54) - base;
    const size_t paths = UINT32_C(0x80184720) - base, ranges = UINT32_C(0x80184768) - base;
    const size_t wave = UINT32_C(0x8019de30) - base;
    if (!disc || !scene || !tiles || bytes < wave + 2 * 1024 ||
        u32le(scene + binding) != room || scene[paths + 4 * 14] != 255 ||
        !u16le(scene + ranges) || !u16le(scene + ranges + 2)) return NULL;
    for (unsigned i = 0; i < 4; ++i) {
        const uint8_t *row = scene + paths + 14 * i;
        if (row[0] > 1 || row[1] > 2) return NULL;
    }
    BfmVillageMineView *view = bfm_village_mine_view_open_field(disc, scene, bytes, room, UINT32_C(0x80182acc), 108, tiles);
    if (view) {
        view->bearings = scene + paths;
        view->distances = scene + ranges;
        view->wave = scene + wave;
    }
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

void bfm_village_mine_view_set_camera(BfmVillageMineView *view, const BfmVillageCamera *camera)
{
    if (view && camera) view->camera = *camera;
}

const BfmBedroomView *bfm_village_mine_view_projection(BfmVillageMineView *view)
{
    if (!view || !bfm_bedroom_field_set_focal(view->field.field, view->camera.focal)) return NULL;
    bfm_bedroom_field_viewpoint(view->field.field, view->camera.eye, view->camera.target, 1, 1);
    return bfm_bedroom_field_view(view->field.field);
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

static int track_angle(int angle, int goal, int divisor, int taper)
{
    int delta = goal - angle;
    if (delta > 2048) delta -= 4096;
    if (delta < -2048) delta += 4096;
    return (angle + track_delta(delta, divisor, taper)) & 4095;
}

void bfm_village_mine_view_follow(BfmVillageMineView *view, const int16_t foot[3], int entering)
{
    if (!view || !view->zones || !foot) return;
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
    view->pitch = track_angle(view->pitch, view->goal_pitch, 10, !in_zone);
    view->yaw = track_angle(view->yaw, view->goal_yaw, 10, !in_zone);
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

/* The gallery divides its length by directed boundaries. At either end of
 * a boundary its test extends along the adjacent axis. */
static int before_boundary(const uint8_t *row, int x, int z)
{
    int ax = s16le(row + 2), az = s16le(row + 4);
    int bx = s16le(row + 6), bz = s16le(row + 8);
    if (row[0] == 255) return 1;
    int reversed = row[0];
    if (x < (reversed ? bx : ax)) return 1;
    if (x > (reversed ? ax : bx)) return 0;
    if (z < bz) return !reversed;
    if (z > az) return reversed;
    return (int64_t)(bx - ax) * (z - az) < (int64_t)(bz - az) * (x - ax);
}

static int sin_q12(unsigned angle)
{
    return (int)lround(4096.0 * sin((angle & 4095u) * 0.0015339807878856412));
}

static int corridor_bearing(const uint8_t *rows, const int16_t foot[3], unsigned facing)
{
    unsigned i = 0;
    while (!before_boundary(rows + 14*i, foot[0], foot[2])) ++i;
    const uint8_t *row = rows + 14*i;
    if (row[1] == 2) return 2048;
    int x = foot[0] - sin_q12(facing) / 128 - s16le(row + 10);
    int z = foot[2] - sin_q12(facing + 1024) / 128 - s16le(row + 12);
    int yaw = row[1] ? bfm_waypoint_heading(-x, -z) : bfm_waypoint_heading(x, z);
    if (yaw < 1152) return 1152;
    return yaw > 2944 ? 2944 : yaw;
}

static int gallery_sine(const uint8_t *quarter, unsigned angle)
{
    unsigned phase = angle & 1023;
    if (angle & 1024) phase = 1023 - phase;
    int sample = s16le(quarter + 2*phase);
    return angle & 2048 ? -sample : sample;
}

static int mul_q15(int a, int b)
{
    int n = a*b;
    return n/32768 - (n%32768 < 0);
}

void bfm_village_mine_corridor_view_follow(BfmVillageMineView *view, const int16_t foot[3],
    unsigned player_yaw, int entering, int shoulder_pressed)
{
    if (!view || !view->bearings || !foot) return;
    BfmVillageCamera *camera = &view->camera;
    if (entering) {
        view->zoom = 0;
        view->yaw = 0;
        camera->focal = 400;
        camera->radius = view->goal_radius = 1000;
        camera->following = 1;
    }
    if (shoulder_pressed) {
        view->zoom ^= 1;
        view->goal_radius = u16le(view->distances + 2*view->zoom);
    }
    int goal = corridor_bearing(view->bearings, foot, player_yaw);
    camera->yaw = track_angle(view->yaw, goal, 4, 0);
    camera->radius += track_delta(view->goal_radius - camera->radius, 4, 1);
    camera->target[0] = foot[0];
    camera->target[1] = (foot[1] < 0 ? foot[1] : 0) - 80;
    camera->target[2] = foot[2] < -128 ? foot[2] : -128;

    /* This gallery builds its arm from its own mounted 15-bit quarter wave.
     * Both pitch and roll contribute a rounded unit coefficient. */
    const uint8_t *wave = view->wave;
    int cp = gallery_sine(wave, 227 + 1024), sp = gallery_sine(wave, 227);
    int unit = gallery_sine(wave, 1024), sy = gallery_sine(wave, camera->yaw);
    int cy = gallery_sine(wave, camera->yaw + 1024);
    int arm[3] = {mul_q15(mul_q15(cp, unit), sy), -mul_q15(sp, unit), mul_q15(cp, cy)};
    for (unsigned i = 0; i < 3; ++i)
        camera->eye[i] = camera->target[i] + mul_q15(arm[i], camera->radius);
    view->yaw = entering ? goal : camera->yaw;
    if (entering) camera->radius = view->goal_radius;
}

void bfm_village_mine_corridor_glow(unsigned world_tick, uint8_t lights[3], uint8_t halo[3])
{
    int pulse = sin_q12((world_tick * 56u) & 2040u);
    int rise = pulse * 100 / 4096;
    lights[0] = (uint8_t)(100 + rise);
    lights[1] = (uint8_t)(130 + rise);
    lights[2] = (uint8_t)(100 + pulse * 110 / 4096);
    for (unsigned i = 0; i < 3; ++i) halo[i] = (uint8_t)(lights[i] - 50);
}

int bfm_village_mine_view_draw(BfmVillageMineView *view, const BfmBedroomRoot *root,
    const BfmBedroomPose *pose, const uint8_t flags[64], unsigned age, unsigned scale_q12,
    uint32_t pixels[240][320],
    int (*submit)(void *, BfmBedroomFigure *, const BfmBedroomView *), void *actor)
{
    if (!pose || !flags) return 0;
    return bfm_village_mine_view_draw_parts(view, root, pose, flags, age,
        bfm_village_field_visible(pose, flags), scale_q12, pixels, submit, actor);
}

int bfm_village_mine_view_draw_parts(BfmVillageMineView *view, const BfmBedroomRoot *root,
    const BfmBedroomPose *pose, const uint8_t flags[64], unsigned age, unsigned visible,
    unsigned scale_q12, uint32_t pixels[240][320],
    int (*submit)(void *, BfmBedroomFigure *, const BfmBedroomView *), void *actor)
{
    if (!view || !root || !pose || !flags || !pixels) return 0;
    for (unsigned y = 0; y < 240; ++y)
        for (unsigned x = 0; x < 320; ++x) pixels[y][x] = UINT32_C(0xff000000);
    if (!bfm_village_field_prepare_scaled(&view->field, &view->camera, root, pose, flags,
        age, visible, scale_q12, view->tiles, &pixels[0][0])) return 0;
    const BfmBedroomView *projection = bfm_bedroom_field_view(view->field.field);
    if (submit && !submit(actor, view->field.hero, projection)) return 0;
    bfm_bedroom_field_draw(view->field.field, view->tiles, &pixels[0][0],
                           bfm_bedroom_figure_draw_to, view->field.hero);
    return 1;
}
