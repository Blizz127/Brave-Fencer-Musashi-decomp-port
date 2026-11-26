/* SPDX-License-Identifier: MIT */
#include "forest_glows.h"
#include "forest_opening_data.h"
#include "forest_shore_motion.h"
#include "../pc_port/forest_player_bytes.h"
#include <math.h>
#include <stdlib.h>

static int load_list(BfmForestGlow *lights, unsigned count, const BfmForestField *field,
                     uint32_t address)
{
    /* The sentinel is part of both mounted lists. */
    for (unsigned i = 0, found = 0; i < 128 && address <= UINT32_MAX - 20; ++i, address += 20) {
        const uint8_t *row = bfm_forest_room_bytes(field, address, 20);
        if (!row) return 0;
        unsigned kind = u16le(row + 6);
        if (!kind) return found == count;
        if (kind != 8) continue;
        unsigned variant = u16le(row + 8);
        unsigned path = variant >> 8;
        if (found == count || path > 3) return 0;
        const uint8_t *route = bfm_forest_room_bytes(field, UINT32_C(0x8018a940) + 16 * path, 16);
        if (!route) return 0;
        BfmForestGlow *light = &lights[found++];
        light->placement_z = s16le(row + 4);
        light->start_yaw = s16le(row + 12);
        light->first_target = variant & 1;
        for (unsigned goal = 0; goal < 2; ++goal)
            for (unsigned axis = 0; axis < 3; ++axis)
                light->route[goal][axis] = s16le(route + goal * 8 + axis * 2);
    }
    return 0;
}

int bfm_forest_glows_load(BfmForestGlows *lights, const BfmForestField *field,
                          const uint8_t *room_descriptor)
{
    if (!lights || !field || !room_descriptor ||
        u32le(room_descriptor + 0xbc) != UINT32_C(0x8013e298)) return 0;
    BfmForestGlows parsed = {0};
    const uint8_t *velocity = bfm_forest_room_bytes(field, UINT32_C(0x8018a9d4), 12);
    if (!velocity || u32le(velocity) || u32le(velocity + 4) || u16le(velocity + 8)) return 0;
    int speed = s16le(velocity + 10);
    if (speed >= 0 || speed < -16) return 0;
    parsed.forward = speed * 65536;
    parsed.halo = bfm_forest_room_bytes(field, UINT32_C(0x8018a9d0), 76);
    if (!parsed.halo || s16le(parsed.halo) <= 0 || s16le(parsed.halo + 2) != 256)
        return 0;
    for (unsigned i = 0; i < 16; ++i) {
        if (parsed.halo[28 + i] > 4 || parsed.halo[44 + i] > 4 || parsed.halo[60 + i] > 2)
            return 0;
    }
    if (!load_list(parsed.glows, BFM_FOREST_ENTRY_GLOWS, field, u32le(room_descriptor + 0xb4)) ||
        !load_list(parsed.glows + BFM_FOREST_ENTRY_GLOWS,
                   BFM_FOREST_GLOWS - BFM_FOREST_ENTRY_GLOWS, field, u32le(room_descriptor + 0xb8))) return 0;
    *lights = parsed;
    return 1;
}

static void start(BfmForestGlows *lights, unsigned index, BfmGameplayRun *run)
{
    BfmForestGlow *light = &lights->glows[index];
    light->active = 1;
    light->target = (uint8_t)light->first_target;
    light->seed_phase = bfm_gameplay_random(run) & 63;
    light->phase = bfm_gameplay_random(run) & 63;
    light->key = 0;
    light->pitch = 0;
    light->yaw = light->start_yaw;
    for (unsigned axis = 0; axis < 3; ++axis) {
        light->jitter[axis] = 0;
        light->position[axis] = (int32_t)light->route[light->target][axis] * 65536;
    }
    bfm_forest_actor_root((unsigned)light->yaw, light->route[light->target], &light->figure);
    lights->order[lights->count++] = index;
}

void bfm_forest_glows_start(BfmForestGlows *lights, BfmGameplayRun *run)
{
    if (!lights || !run || lights->started) return;
    lights->started = 1;
    for (unsigned i = 0; i < BFM_FOREST_ENTRY_GLOWS; ++i)
        start(lights, i, run);
}

static int floor_q(int64_t value, int denominator)
{
    return (int)(value / denominator - (value % denominator < 0));
}

static void move(BfmForestGlow *light, int32_t forward, BfmGameplayRun *run)
{
    if (!(light->phase & 1)) {
        for (unsigned axis = 0; axis < 3; ++axis)
            light->jitter[axis] = (int16_t)((bfm_gameplay_random(run) & 15) - 7);
    }
    int16_t current[3];
    int delta[3];
    int64_t distance = 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        current[axis] = (int16_t)floor_q(light->position[axis], 65536);
        delta[axis] = light->route[light->target][axis] - current[axis];
        distance += (int64_t)delta[axis] * delta[axis];
    }
    if (distance <= 32 * 32) {
        light->target ^= 1;
    } else {
        unsigned goal_yaw = (bfm_forest_shore_bearing(-delta[2], delta[0]) + 3072) & 4095;
        const double radians = 6.28318530717958647693 / 4096;
        int sin_yaw = (int)lround(4096 * sin(goal_yaw * radians));
        int cos_yaw = (int)lround(4096 * cos(goal_yaw * radians));
        int horizontal = -floor_q((int64_t)delta[0] * sin_yaw + (int64_t)delta[2] * cos_yaw, 4096);
        int pitch = (int)bfm_forest_shore_bearing(delta[1], horizontal);
        light->pitch = (int16_t)(pitch > 2048 ? pitch - 4096 : pitch);
        int turn = (int)goal_yaw - ((unsigned)light->yaw & 4095);
        if (turn > 2048) turn -= 4096;
        if (turn < -2048) turn += 4096;
        int step = turn / 23;
        if (turn && !step) step = turn > 0 ? 1 : -1;
        light->yaw = (int16_t)(light->yaw + step);

        /* The model follows the actor's pre-movement root with its own jitter. */
        BfmForestTransform pitch_axis, yaw_axis;
        bfm_forest_actor_root((unsigned)abs(light->pitch), current, &pitch_axis);
        bfm_forest_actor_root((unsigned)abs(light->yaw), current, &yaw_axis);
        int sy = yaw_axis.rotation[0][2] * (light->yaw < 0 ? -1 : 1);
        int sx = pitch_axis.rotation[0][2] * (light->pitch < 0 ? -1 : 1);
        int cy = yaw_axis.rotation[2][2], cx = pitch_axis.rotation[2][2];
        /* Form the signed angles before fixed-point products are rounded. */
        light->figure = (BfmForestTransform){.rotation = {
            {cy, floor_q((int64_t)sy * sx, 4096), floor_q((int64_t)sy * cx, 4096)},
            {0, cx, -sx},
            {-sy, floor_q((int64_t)cy * sx, 4096), floor_q((int64_t)cy * cx, 4096)}
        }};
        for (unsigned axis = 0; axis < 3; ++axis)
            light->figure.translation[axis] = current[axis] + light->jitter[axis];
    }
    light->key = (uint8_t)((light->key + 1) % BFM_FOREST_GLOW_KEYS);
    ++light->phase;
    for (unsigned axis = 0; axis < 3; ++axis)
        light->position[axis] += floor_q((int64_t)light->figure.rotation[axis][2] * forward, 4096);
    /* At a waypoint the steering pass reuses its basis. The model directly
     * receives the new logical position and current body offset. */
    if (distance <= 32 * 32) {
        int16_t placed[3];
        for (unsigned axis = 0; axis < 3; ++axis)
            placed[axis] = (int16_t)(floor_q(light->position[axis], 65536) + light->jitter[axis]);
        bfm_forest_actor_pitch_yaw((unsigned)light->pitch, (unsigned)light->yaw, placed, &light->figure);
    }
}

void bfm_forest_glows_move(BfmForestGlows *lights, int player_z, BfmGameplayRun *run)
{
    if (!lights || !run) return;
    unsigned count = 0;
    lights->draw_count = 0;
    for (unsigned i = 0; i < lights->count; ++i) {
        unsigned index = lights->order[i];
        BfmForestGlow *light = &lights->glows[index];
        /* The halo projects the logical root before movement. It still draws
         * on a retiring light's last update. */
        BfmForestGlowFrame *frame = &lights->frame[lights->draw_count++];
        for (unsigned axis = 0; axis < 3; ++axis)
            frame->position[axis] = (int16_t)floor_q(light->position[axis], 65536);
        move(light, lights->forward, run);
        frame->phase = light->phase;
        if (abs(floor_q(light->position[2], 65536) - player_z) >= 864)
            light->active = 0;
        else
            lights->order[count++] = index;
    }
    lights->count = count;
}

void bfm_forest_glows_place(BfmForestGlows *lights, int player_z, BfmGameplayRun *run)
{
    if (!lights || !run) return;
    for (unsigned i = BFM_FOREST_ENTRY_GLOWS; i < BFM_FOREST_GLOWS; ++i) {
        BfmForestGlow *light = &lights->glows[i];
        int depth = abs(light->placement_z - player_z);
        if (!light->active && depth >= 705 && depth < 768)
            start(lights, i, run);
    }
}
