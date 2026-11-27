/* SPDX-License-Identifier: MIT */
#include "forest_water_effect.h"
#include "forest_opening_data.h"
#include "forest_pose.h"
#include "forest_shore_motion.h"
#include "../pc_port/forest_player_bytes.h"

#include <limits.h>
#include <math.h>

enum { UNIT = 4096, COLUMN_UPDATES = 31, RING_UPDATES = 32, FLECK_UPDATES = 15 };

static int floor_div(int64_t value, int64_t divisor)
{
    return (int)(value / divisor - (value % divisor < 0));
}

static int load_sprite(BfmForestWaterSprite *sprite, const uint8_t *cell,
                       unsigned x, unsigned y, unsigned depth,
                       uint16_t clut)
{
    unsigned width = cell[2], height = cell[3];
    unsigned u = (x % 64) * (4u >> depth), v = y % 256;
    int left = s16le(cell + 8), top = s16le(cell + 10);
    if (!width || !height || u + width > 256 || v + height > 256 ||
        left + (int)width > INT16_MAX || top + (int)height > INT16_MAX) return 0;
    *sprite = (BfmForestWaterSprite){
        .left = (int16_t)left, .top = (int16_t)top,
        .width = (uint8_t)width, .height = (uint8_t)height, .u = (uint8_t)u, .v = (uint8_t)v,
        .material = {.page = (uint16_t)(x / 64 | (y / 256) << 4 | 1u << 5 | depth << 7),
                     .clut = clut, .count = 4, .textured = 1, .translucent = 1}
    };
    return 1;
}

int bfm_forest_water_effect_load_cells(BfmForestWaterEffect *effect,
                                      const uint8_t *cells, size_t bytes)
{
    BfmForestWaterEffect loaded = {0};
    if (!effect || !cells || bytes < 24 ||
        !load_sprite(&loaded.column, cells, 576, 384, 1, 0x7f50) ||
        !load_sprite(&loaded.ring, cells + 12, 584, 384, 0, 0x4616))
        return 0;
    *effect = loaded;
    return 1;
}

int bfm_forest_water_effect_load(BfmForestWaterEffect *effect, const BfmForestField *field)
{
    if (!field) return 0;
    return bfm_forest_water_effect_load_cells(effect,
        bfm_forest_room_bytes(field, UINT32_C(0x8018a508), 24), 24);
}

/* Normalize all three components before applying the short horizontal
 * offset. A high camera gives a smaller nudge than a level camera. */
static void eye_direction(const int16_t origin[3], const int32_t eye[3], int direction[3])
{
    int64_t delta[3];
    uint64_t length_squared = 0;
    for (unsigned i = 0; i < 3; ++i) {
        delta[i] = (int64_t)eye[i] - origin[i];
        length_squared += (uint64_t)(delta[i] * delta[i]);
    }
    if (!length_squared) {
        direction[0] = direction[1] = direction[2] = 0;
        return;
    }
    int64_t divisor = 8;
    while (length_squared > 255) { length_squared >>= 2; divisor *= 2; }
    while (length_squared < 64) { length_squared <<= 2; divisor /= 2; }
    int gain = (int)sqrt((double)(UINT32_C(1) << 30) / (double)length_squared);
    for (unsigned i = 0; i < 3; ++i)
        direction[i] = floor_div(delta[i] * gain, divisor);
}

int bfm_forest_water_effect_begin(BfmForestWaterEffect *effect, const int16_t xyz[3],
                                 const int32_t eye[3], BfmGameplayRun *run)
{
    if (!effect || !xyz || !eye || !run || effect->active ||
        !effect->column.width || !effect->ring.width) return 0;
    int direction[3];
    eye_direction(xyz, eye, direction);
    unsigned yaw = bfm_forest_shore_bearing(direction[0], direction[2]);
    unsigned pitch = 0u - bfm_forest_shore_bearing(direction[1],
                                  (int)hypot(direction[0], direction[2]));
    bfm_forest_actor_root(yaw, xyz, &effect->column_root);
    effect->column_root.translation[0] += floor_div(direction[0], 1024);
    effect->column_root.translation[2] += floor_div(direction[2], 1024);
    bfm_forest_actor_pitch_yaw(1024, 0, xyz, &effect->ring_root);
    effect->ring_root.translation[1] -= 16;
    bfm_forest_actor_pitch_yaw(pitch, yaw, xyz, &effect->fleck_face);

    for (unsigned i = 0; i < BFM_FOREST_WATER_FLECKS; ++i) {
        unsigned random = bfm_gameplay_random(run);
        unsigned launch_pitch = 0x180 + (random & 0x1fc);
        unsigned launch_yaw = ((random & 0x7f00) >> 6) + ((i & 2) ? 0x200 : 0xc00);
        BfmForestTransform launch;
        bfm_forest_actor_pitch_yaw(launch_pitch, launch_yaw, xyz, &launch);
        for (unsigned axis = 0; axis < 3; ++axis) {
            int forward = 0;
            for (unsigned k = 0; k < 3; ++k)
                forward += effect->column_root.rotation[axis][k] * launch.rotation[k][2];
            effect->flecks[i].position[axis] = xyz[axis];
            effect->flecks[i].velocity[axis] =
                (int16_t)floor_div(floor_div(forward, UNIT) * 10, UNIT);
        }
    }
    effect->height_scale = 0x200;
    effect->height_delta = 0x1000;
    effect->age = 0;
    effect->active = 1;
    return 1;
}

void bfm_forest_water_effect_tick(BfmForestWaterEffect *effect)
{
    if (!effect || !effect->active) return;
    if (effect->age < COLUMN_UPDATES) {
        effect->height_scale += effect->height_delta;
        effect->height_delta -= effect->age < 8 ? 0x200 : 0x51;
    }
    if (effect->age < FLECK_UPDATES) {
        for (unsigned i = 0; i < BFM_FOREST_WATER_FLECKS; ++i) {
            BfmForestWaterFleck *fleck = &effect->flecks[i];
            for (unsigned axis = 0; axis < 3; ++axis)
                fleck->position[axis] += fleck->velocity[axis];
            ++fleck->velocity[1];
        }
    }
    if (++effect->age >= RING_UPDATES) effect->active = 0;
}

static int scaled_camera(const BfmForestCamera *camera, BfmForestTransform model,
                         int x_scale, int y_scale, BfmForestCamera *local)
{
    /* All cards lie in local XY. The ring's rotated Y axis lies along Z. */
    for (unsigned i = 0; i < 3; ++i) {
        model.rotation[i][0] = (int16_t)floor_div(model.rotation[i][0] * x_scale, UNIT);
        model.rotation[i][1] = (int16_t)floor_div(model.rotation[i][1] * y_scale, UNIT);
    }
    return bfm_forest_effect_camera(camera, &model, local);
}

static int project_sprite(const BfmForestCamera *camera, const BfmForestTransform *root,
                          const BfmForestWaterSprite *sprite, int x_scale, int y_scale,
                          unsigned brightness, unsigned slot, BfmForestDustCard *out)
{
    BfmForestCamera local;
    if (!scaled_camera(camera, *root, x_scale, y_scale, &local)) return 0;
    BfmForestDustCard card = {.material = sprite->material, .slot = slot};
    int16_t corners[4][3] = {{0}};
    for (unsigned i = 0; i < 4; ++i) {
        int right = (i & 1) != 0, bottom = (i & 2) != 0;
        corners[i][0] = (int16_t)(sprite->left + right * sprite->width);
        corners[i][1] = (int16_t)(sprite->top + bottom * sprite->height);
        card.vertices[i].u = (uint8_t)(sprite->u + right * (sprite->width - 1));
        card.vertices[i].v = (uint8_t)(sprite->v + bottom * (sprite->height - 1));
        card.vertices[i].rgb = brightness * UINT32_C(0x010101);
    }
    if (!bfm_forest_project_effect_quad(&local, corners, &card)) return 0;
    *out = card;
    return 1;
}

static size_t project_fleck(const BfmForestWaterEffect *effect, unsigned index,
                            const BfmForestCamera *camera, BfmForestDustCard *out,
                            size_t capacity)
{
    BfmForestTransform root = effect->fleck_face;
    for (unsigned i = 0; i < 3; ++i) root.translation[i] = effect->flecks[index].position[i];
    int scale = index == 1 ? 0xe00 : 0xa00;
    BfmForestCamera local;
    if (!scaled_camera(camera, root, scale, scale, &local)) return 0;
    BfmForestDustCard card = {.material = {.page = 32, .count = 4, .translucent = 1},
                              .slot = index + 2};
    unsigned blue = 192 - 8 * effect->age;
    uint32_t rgb = (blue << 16) | ((blue / 4) << 8) | (blue / 4);
    size_t count = 0;
    for (unsigned shape = 0; shape < 2 && count < capacity; ++shape) {
        int16_t corners[4][3] = {{0}};
        for (unsigned i = 0; i < 4; ++i) {
            int x = (i & 1) ? 1 : -1, y = (i & 2) ? 1 : -1;
            corners[i][0] = (int16_t)(shape ? (x + y) * 7 / 2 : 5 * x);
            corners[i][1] = (int16_t)(shape ? (y - x) * 7 / 2 : 5 * y);
            card.vertices[i].rgb = rgb;
        }
        if (bfm_forest_project_effect_quad(&local, corners, &card)) out[count++] = card;
    }
    return count;
}

size_t bfm_forest_water_effect_project(const BfmForestWaterEffect *effect,
                                      const BfmForestCamera *camera,
                                      BfmForestDustCard *out, size_t capacity)
{
    if (!effect || !effect->active || !camera || !out || !capacity) return 0;
    size_t count = 0;
    if (effect->age < COLUMN_UPDATES)
        count += project_sprite(camera, &effect->column_root, &effect->column,
                                0x4000, effect->height_scale, 128 - 2 * effect->age, 0, out);
    if (count < capacity) {
        int ring_scale = 0x3000 + (int)effect->age * 0x1e0;
        count += project_sprite(camera, &effect->ring_root, &effect->ring, ring_scale,
                                ring_scale, 128 - 3 * effect->age, 1, out + count);
    }
    if (effect->age < FLECK_UPDATES)
        for (unsigned i = 0; i < BFM_FOREST_WATER_FLECKS && count < capacity; ++i)
            count += project_fleck(effect, i, camera, out + count, capacity - count);
    return count;
}
