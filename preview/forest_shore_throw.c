/* SPDX-License-Identifier: MIT */
#include "forest_shore_throw.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"

#include <limits.h>
#include <string.h>

enum { FIRST_FLIGHT, SECOND_FLIGHT, LANDING, FINISHED };
static const uint32_t ready_clip = UINT32_C(0x80196f3c);

int bfm_forest_shore_release_begin(BfmForestShoreThrow *motion, const BfmForestField *field,
                                   const int32_t position[3], int attacker_yaw, unsigned kind)
{
    if (!motion || !field || !position || (kind != 9 && kind != 17)) return 0;
    uint32_t velocity_record = kind == 9 ? UINT32_C(0x80186e8c) : UINT32_C(0x80186e98);
    const uint8_t *launch = bfm_forest_room_bytes(field, velocity_record, 12);
    const uint8_t *response = bfm_forest_room_bytes(field, UINT32_C(0x8018af20), 88);
    if (!launch || !response) return 0;
    BfmForestShoreThrow next = { .yaw = (int16_t)(((unsigned)attacker_yaw + 2048) & 4095) };
    for (unsigned i = 0; i < 11; ++i) {
        unsigned tag = u32le(response + 8 * i);
        uint32_t value = u32le(response + 8 * i + 4);
        if (!tag) break;
        if (tag == 32) next.tumble_clip = value;
        if (tag == 46 && value <= INT16_MAX) next.probe_height = (int16_t)value;
    }
    if (!next.probe_height || next.tumble_clip != UINT32_C(0x80197514)) return 0;
    next.clip = next.tumble_clip;
    memcpy(next.position, position, sizeof next.position);
    const int16_t zero[3] = {0};
    BfmForestTransform facing;
    bfm_forest_actor_root((unsigned)next.yaw, zero, &facing);
    for (unsigned row = 0; row < 3; ++row) {
        int64_t dot = 0;
        for (unsigned axis = 0; axis < 3; ++axis) {
            int32_t speed = (int32_t)u32le(launch + 4 * axis);
            dot += (int64_t)facing.rotation[row][axis] * speed;
        }
        int64_t q = dot / 4096 - (dot % 4096 < 0);
        if (q < INT32_MIN || q > INT32_MAX) return 0;
        next.velocity[row] = (int32_t)q;
    }
    *motion = next;
    return 1;
}

int bfm_forest_shore_throw_begin(BfmForestShoreThrow *motion, const BfmForestField *field,
                                 const int32_t position[3], int attacker_yaw)
{
    return bfm_forest_shore_release_begin(motion, field, position, attacker_yaw, 9);
}

static int whole(int32_t q16)
{
    return q16 / 65536 - (q16 % 65536 < 0);
}

static int clear_release_probe(BfmForestShoreThrow *motion, const BfmForestField *field)
{
    int16_t foot[3];
    for (unsigned axis = 0; axis < 3; ++axis)
        foot[axis] = (int16_t)whole(motion->position[axis]);
    int depth = motion->probe_height, floor;
    if (foot[1] > INT16_MAX - depth) return 0;
    int16_t end[] = {foot[0], (int16_t)(foot[1] + depth), foot[2]};
    if (!bfm_forest_thrown_probe(field, foot, end, 0, &floor) || end[1] < INT16_MIN + depth)
        return 0;
    end[1] -= depth;
    for (unsigned axis = 0; axis < 3; ++axis)
        motion->position[axis] = end[axis] * 65536 +
            (int)((uint32_t)motion->position[axis] & 65535);
    return 1;
}

static int move(BfmForestShoreThrow *motion, const BfmForestField *field, int *floor, int *wall)
{
    int offset = motion->phase == LANDING ? 0 : motion->probe_height;
    int16_t from[3], to[3];
    if (motion->velocity[1] > INT32_MAX - 98304) return 0;
    motion->velocity[1] += 98304;
    for (unsigned axis = 0; axis < 3; ++axis) {
        int at = whole(motion->position[axis]);
        int64_t q = (int64_t)motion->position[axis] + motion->velocity[axis];
        if (q < INT32_MIN || q > INT32_MAX) return 0;
        motion->position[axis] = (int32_t)q;
        int end = whole((int32_t)q);
        if (axis == 1) {
            at += offset;
            end += offset;
            if (at > INT16_MAX || end > INT16_MAX) return 0;
        }
        from[axis] = (int16_t)at;
        to[axis] = (int16_t)end;
    }
    if (bfm_forest_thrown_probe(field, from, to, motion->grounded, floor)) {
        *wall = 0;
    } else {
        int x = to[0], z = to[2];
        BfmForestContact surface;
        if (!bfm_forest_air_step(field, from, to, &surface, floor)) return 0;
        if ((*floor || motion->grounded) && (surface.flags || surface.slope != 0x2000 ||
            surface.normal[0] || surface.normal[2] || surface.normal[1] >= 0)) return 0;
        *wall = (x != to[0] || z != to[2]);
        if (motion->grounded) {
            /* Once the body has landed it follows the bank under its feet. */
            if (from[1] != surface.height) return 0;
            to[1] = surface.height;
            *floor = 1;
        }
    }
    for (unsigned axis = 0; axis < 3; ++axis) {
        int end = to[axis] - (axis == 1 ? offset : 0);
        if (end < INT16_MIN) return 0;
        motion->position[axis] = end * 65536 + (int)((uint32_t)motion->position[axis] & 65535);
    }
    if (*floor) motion->velocity[1] = 0;
    if (*wall) motion->velocity[0] = motion->velocity[2] = 0;
    return 1;
}

BfmForestShoreThrowStep bfm_forest_shore_throw_tick(BfmForestShoreThrow *motion,
                                                    const BfmForestField *field)
{
    if (!motion || !field || motion->phase > LANDING) return BFM_SHORE_THROW_UNRESOLVED;
    BfmForestShoreThrow next = *motion;
    if (next.phase == FIRST_FLIGHT && !next.ticks && !clear_release_probe(&next, field))
        return BFM_SHORE_THROW_UNRESOLVED;
    next.key = (next.key + 1) % (next.clip == next.tumble_clip ? 15 : 20);
    if (next.phase == LANDING) {
        for (unsigned axis = 0; axis < 3; axis += 2)
            next.velocity[axis] = (int32_t)((int64_t)next.velocity[axis] * 15 / 16);
    } else {
        if (next.ticks >= 60) return BFM_SHORE_THROW_UNRESOLVED;
        next.pitch = (int16_t)(next.pitch - 256);
    }
    ++next.ticks;
    int floor, wall;
    int32_t previous_speed = next.velocity[1];
    if (!move(&next, field, &floor, &wall)) return BFM_SHORE_THROW_UNRESOLVED;
    if (next.phase == LANDING) {
        if (floor && !next.grounded) {
            next.grounded = 1;
            next.clip = ready_clip;
            next.key = 0;
        }
        if (next.ticks >= 17) {
            if (!floor) return BFM_SHORE_THROW_UNRESOLVED;
            next.phase = FINISHED;
            next.landed_key = next.key;
            next.key = 0;
        }
    } else if (floor || wall) {
        if (next.phase == FIRST_FLIGHT) {
            next.phase = SECOND_FLIGHT;
            if (floor) next.velocity[1] = -13 * 65536;
        } else {
            next.phase = LANDING;
            next.pitch = 0;
            if (floor) next.velocity[1] = -(previous_speed / 3);
        }
        next.ticks = 0;
    }
    *motion = next;
    return next.phase == FINISHED ? BFM_SHORE_THROW_RECOVER : BFM_SHORE_THROW_MOVING;
}
