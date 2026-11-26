/* SPDX-License-Identifier: MIT */
#include "castle_drawbridge_view.h"
#include "tower_sky.h"
#include "forest_pose.h"

#include <stdlib.h>

struct BfmCastleDrawbridgeView {
    BfmForestField *field;
    BfmTowerSky *sky;
};

BfmCastleDrawbridgeView *bfm_castle_drawbridge_view_open(const MusashiDiscMedia *disc)
{
    BfmCastleDrawbridgeView *view = calloc(1, sizeof *view);
    if (!view) return NULL;
    view->field = bfm_forest_field_open(disc, 0x300d);
    size_t size = 0;
    const uint8_t *scene = bfm_forest_field_scene(view->field, &size);
    if (scene)
        view->sky = bfm_castle_drawbridge_sky_open(scene, size, bfm_forest_field_tiles(view->field));
    if (!view->sky) {
        bfm_castle_drawbridge_view_free(view);
        return NULL;
    }
    return view;
}

void bfm_castle_drawbridge_view_free(BfmCastleDrawbridgeView *view)
{
    if (view) {
        bfm_tower_sky_free(view->sky);
        bfm_forest_field_free(view->field);
        free(view);
    }
}

BfmForestField *bfm_castle_drawbridge_view_room(BfmCastleDrawbridgeView *view)
{
    return view ? view->field : NULL;
}

typedef struct {
    unsigned tick;
    int16_t eye[3], yaw, pitch, focal;
} BridgeCameraKey;

/* The entrance camera descends over the gate and looks along the moat.
 * These poses fit the projected drawbridge against the game's visible
 * crossing. Interpolate between the observed views, holding the last
 * view while Musashi walks under the gate. */
static const BridgeCameraKey crossing[] = {
    { 17, {  54, -2434, -87}, -1453, 996, 499},
    { 21, {  84, -2424, -84}, -1311, 984, 508},
    { 39, { 287, -2099, -59}, -1113, 863, 494},
    { 61, { 624, -1626, -26}, -1061, 542, 495},
    {106, {1162,  -913,  29}, -1042,  -5, 506},
    {151, {1158,  -896,  32}, -1043, -13, 502}
};

static int between(int begin, int end, int elapsed, int span)
{
    return span ? begin + (end - begin) * elapsed / span : begin;
}

static void crossing_camera(unsigned tick, BfmForestCamera *camera)
{
    const BridgeCameraKey *a = crossing, *b = crossing;
    if (tick > a->tick) {
        for (size_t i = 1; i < sizeof crossing / sizeof crossing[0]; ++i) {
            b = &crossing[i];
            if (tick < b->tick) break;
            a = b;
        }
    }
    int span = (int)(b->tick - a->tick);
    int elapsed = span ? (int)(tick - a->tick) : 0;
    int16_t eye[3];
    for (unsigned i = 0; i < 3; ++i)
        eye[i] = (int16_t)between(a->eye[i], b->eye[i], elapsed, span);
    int yaw = between(a->yaw, b->yaw, elapsed, span);
    int pitch = between(a->pitch, b->pitch, elapsed, span);
    BfmForestTransform orientation;
    bfm_forest_actor_pitch_yaw((unsigned)-pitch, (unsigned)yaw, eye, &orientation);
    camera->projection = (uint16_t)between(a->focal, b->focal, elapsed, span);
    for (unsigned axis = 0; axis < 3; ++axis) {
        int position = 0;
        for (unsigned j = 0; j < 3; ++j) {
            camera->rotation[axis][j] = orientation.rotation[j][axis];
            position -= eye[j] * camera->rotation[axis][j];
        }
        camera->translation[axis] = position / 4096 - (position % 4096 < 0);
    }
}

int bfm_castle_drawbridge_view_draw(BfmCastleDrawbridgeView *view, unsigned room_tick,
                                    const BfmForestPose *pose, const BfmForestTransform *root,
                                    uint32_t visible_parts, uint32_t pixels[240][320])
{
    if (!view || !pixels || !pose || !root) return 0;
    BfmForestCamera camera;
    crossing_camera(room_tick, &camera);
    return bfm_tower_sky_draw(view->sky, &camera, pixels) &&
           bfm_forest_draw_player_buffer(view->field, &camera, pose, root,
                                         visible_parts, room_tick & 1, pixels);
}
