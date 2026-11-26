/* SPDX-License-Identifier: MIT */
#include "tower_escape.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

enum { Q = 256, OBSTACLES = 26, ROCK = 69, TRUNK = 70 };

static void enter(BfmTowerEscape *e, BfmTowerEscapePhase phase)
{
    e->phase = phase;
    e->age = 0;
}

static void read_point(const uint8_t *record, int16_t out[3])
{
    for (unsigned axis = 0; axis < 3; ++axis) out[axis] = s16le(record + axis * 2);
}

static void place(BfmTowerEscape *e, const int16_t pos[3])
{
    e->x = pos[0] * Q;
    e->y = pos[1] * Q;
    e->z = pos[2] * Q;
}

static int whole(int n)
{
    return n / Q - (n % Q < 0);
}

static int load_course(BfmTowerEscape *next, const BfmForestField *field)
{
    const uint8_t *exit = bfm_forest_field_entrance(field, 1);
    const uint8_t *marks = bfm_forest_room_bytes(field, UINT32_C(0x801e6c84), 32);
    const uint8_t *lane = bfm_forest_room_bytes(field, UINT32_C(0x8018edc8), 8);
    next->placements = bfm_forest_room_bytes(field, UINT32_C(0x8018f07c), OBSTACLES * 12);
    next->rock_bounds = bfm_forest_room_bytes(field, UINT32_C(0x8018f3a0), 12);
    next->trunk_bounds = bfm_forest_room_bytes(field, UINT32_C(0x8018f3b8), 12);
    if (!exit || !marks || !lane || !next->placements || !next->rock_bounds || !next->trunk_bounds)
        return 0;
    bfm_forest_decode_transfer(exit, &next->destination);
    if (next->destination.room != 0x300d || next->destination.parameter || next->destination.world) return 0;
    for (unsigned i = 0; i < 4; ++i) read_point(marks + 8 * i, next->marks[i]);
    read_point(lane, next->lane);
    return 1;
}

int bfm_tower_escape_begin(BfmTowerEscape *e, const BfmForestField *field,
                           const int16_t position[3], int16_t yaw)
{
    if (!e || e->phase != BFM_ESCAPE_IDLE || !field || !position) return 0;
    BfmTowerEscape next = {0};
    if (!load_course(&next, field)) return 0;
    memcpy(next.position, position, sizeof next.position);
    place(&next, position);
    next.yaw = yaw;
    if (!bfm_tower_escape_head_begin(&next.head, field)) return 0;
    enter(&next, BFM_ESCAPE_RETREAT);
    *e = next;
    return 1;
}

int bfm_tower_escape_resume(BfmTowerEscape *e, const BfmForestField *field)
{
    if (!e || !field || e->phase != BFM_ESCAPE_CAUGHT ||
        (e->checkpoint != BFM_ESCAPE_UPPER_CHECKPOINT && e->checkpoint != BFM_ESCAPE_LOWER_CHECKPOINT))
        return 0;
    /* The active summit door list includes both chase entries after the lift and castle. */
    const uint8_t *slot = bfm_forest_room_bytes(field, UINT32_C(0x8018eab4) + 4 * e->checkpoint, 4);
    const uint8_t *door = slot ? bfm_forest_room_bytes(field, u32le(slot), 36) : NULL;
    if (!door || u16le(door) != 0x300b || door[2] != e->checkpoint || door[3]) return 0;
    BfmTowerEscape next = {.checkpoint = e->checkpoint};
    if (!load_course(&next, field)) return 0;
    read_point(door + 16, next.position);
    place(&next, next.position);
    next.yaw = s16le(door + 22);
    int lower = next.checkpoint == BFM_ESCAPE_LOWER_CHECKPOINT;
    next.forward_speed = (lower ? 11 : 8) * Q;
    enter(&next, lower ? BFM_ESCAPE_LOWER : BFM_ESCAPE_UPPER);
    bfm_tower_escape_head_resume(&next.head, lower);
    *e = next;
    return 1;
}

static int obstacle(const BfmTowerEscape *e, int x, int y, int from_z, int to_z)
{
    for (unsigned i = 0; i < OBSTACLES; ++i) {
        const uint8_t *row = e->placements + 12 * i;
        unsigned kind = u16le(row + 6);
        if (kind != ROCK && kind != TRUNK) continue;
        const uint8_t *box = kind == ROCK ? e->rock_bounds : e->trunk_bounds;
        int dx = x - s16le(row), dy = y - s16le(row + 2), z = s16le(row + 4);
        int head_z = e->head.position[2];
        if (head_z > z + 1040 || head_z < z + (kind == ROCK ? 128 : 16)) continue;
        if (dx < s16le(box) || dx > s16le(box + 2) ||
            dy < s16le(box + 4) || dy > s16le(box + 6) ||
            from_z < z + s16le(box + 10) || to_z > z + s16le(box + 10)) continue;
        return z + s16le(box + 10) + 2;
    }
    return to_z;
}

/* Negative Z points downhill. The camera looks back up the course. The room
 * keeps a forward stride even when the player's only input is lateral. */
static void run_downhill(BfmTowerEscape *e, const BfmForestField *field,
                         int horizontal, int cross_edge, int cross_held)
{
    int upper = e->phase == BFM_ESCAPE_UPPER;
    int from_z = whole(e->z);
    if (upper && e->upper_lip) {
        const int16_t ledge[3] = {0, -1728, 7295};
        place(e, ledge);
        e->airborne = e->vertical_speed = 0;
        enter(e, BFM_ESCAPE_DROP);
        return;
    }
    /* The room first hands the stride to the ordinary action owner. A
     * directed landing also consumes one update before running resumes. */
    if (e->age <= 2 || e->landing) {
        e->action = e->age > 1 || e->landing;
        e->landing = 0;
        return;
    }
    if (upper) horizontal = 0;
    else if (from_z >= e->lane[2] + 17) {
        int goal = e->lane[0];
        /* The scripted alignment reaches the mark over the remaining slope. */
        int distance = from_z - e->lane[2];
        int step = distance > 32 ? (goal * Q - e->x) * 11 / distance : goal * Q - e->x;
        e->x += step;
        horizontal = 0;
    }
    e->yaw = horizontal > 0 ? 3840 : horizontal < 0 ? 256 : 0;
    if (e->forward_speed < 11 * Q) e->forward_speed += Q / 16;
    if (cross_edge && !e->airborne) {
        e->airborne = 1;
        e->vertical_speed = -18 * Q;
        e->action = 2;
        return;
    }
    int lateral_speed = (e->airborne ? 11 * Q + Q / 2 : e->forward_speed + Q / 16) * 2896 / 4096;
    int next_x = e->x + horizontal * lateral_speed;
    int next_z = e->z - (e->airborne ? 11 * Q + Q / 2 : e->forward_speed + Q / 16);
    int16_t step[3] = {(int16_t)whole(next_x), e->position[1], (int16_t)whole(next_z)};
    bfm_forest_clip_field_walls(field, e->position, step);
    next_x += (step[0] - whole(next_x)) * Q;
    next_z += (step[2] - whole(next_z)) * Q;
    BfmForestContact ground;
    int supported = bfm_forest_ground(field, step[0], step[1] - 32, step[2], &ground);
    int projected = 0;
    if (!e->airborne && supported) {
        int16_t end[3] = {(int16_t)whole(next_x), e->position[1], (int16_t)whole(next_z)};
        if (bfm_forest_escape_floor_step(field, e->position, end)) {
            next_x += (end[0] - whole(next_x)) * Q;
            next_z += (end[2] - whole(next_z)) * Q;
            ground.height = end[1];
            projected = 1;
        } else {
            if (ground.height < whole(e->y) - 24) {
                next_x = e->x;
                next_z = e->z;
                ground.height = whole(e->y);
            }
        }
    }
    if (e->airborne || !supported || ground.height - whole(e->y) > 40) {
        e->airborne = 1;
        e->vertical_speed += e->vertical_speed < 0 && !cross_held ? 21 * Q / 8 : 13 * Q / 8;
        e->y += e->vertical_speed;
        e->action = e->vertical_speed < 0 ? 2 : 3;
        if (supported && e->vertical_speed >= 0 && e->y >= ground.height * Q) {
            e->airborne = 0;
            e->landing = 1;
            e->vertical_speed = 0;
        }
    }
    if (!e->airborne && supported) {
        if (!projected && ground.height > whole(e->y)) next_z -= (ground.height - whole(e->y)) * Q / 2;
        e->y = ground.height * Q;
        e->action = e->landing ? 3 : 1;
    }
    e->x = next_x;
    int accepted = upper ? whole(next_z) : obstacle(e, whole(next_x), whole(e->y), from_z, whole(next_z));
    e->z = accepted == whole(next_z) ? next_z : accepted * Q;
    if (upper && whole(e->z) < 10464) {
        e->upper_lip = 1;
    } else if (!upper) {
        if (!e->airborne && supported && (ground.flags & 255) == 2) enter(e, BFM_ESCAPE_CAUGHT);
    }
}

/* The scripted ledge lands on bare tower terrain. The woodland player jump
 * surface filter does not own this material. Resolve the contact against the
 * current summit grid, including its sloping runout. */
static int ledge_vertical_step(BfmTowerEscape *e, const BfmForestField *field, int32_t next_y)
{
    BfmForestContact ground;
    if (!bfm_forest_ground(field, e->position[0], e->position[1] - 32, e->position[2], &ground))
        return 0;
    int32_t floor = ground.height * Q;
    if (next_y >= floor && next_y >= e->y) {
        int excess = whole(next_y - floor);
        e->x += excess * ground.normal[0] * Q / 4096;
        e->z += excess * ground.normal[2] * Q / 4096;
        e->y = floor;
        return 2;
    }
    e->y = next_y;
    return 1;
}

int bfm_tower_escape_tick(BfmTowerEscape *e, const BfmForestField *field,
                          int horizontal, int cross_edge, int cross_held)
{
    if (!e || !field || horizontal < -1 || horizontal > 1 || e->phase == BFM_ESCAPE_IDLE) return 0;
    ++e->age;
    switch (e->phase) {
    case BFM_ESCAPE_RETREAT: {
        if (e->age == 1) break;
        int goal = e->marks[0][0] * Q;
        int delta = goal - e->x;
        e->action = 1;
        e->yaw = delta < 0 ? 1024 : 3072;
        if (abs(delta) < 16 * Q) {
            place(e, e->marks[0]);
            e->action = 0;
            enter(e, BFM_ESCAPE_WAKE);
        } else if (e->age > 2) e->x += (delta < 0 ? -11 : 11) * Q;
        break;
    }
    case BFM_ESCAPE_WAKE:
        if (e->age >= 9 + 65 + 65 + 17) enter(e, BFM_ESCAPE_BREAK);
        break;
    case BFM_ESCAPE_BREAK:
        e->action = 1;
        e->yaw = 3072;
        if (e->age > 1) e->x += 11 * Q;
        if (whole(e->x) >= e->marks[0][0] + 88) enter(e, BFM_ESCAPE_FALL);
        break;
    case BFM_ESCAPE_FALL:
        if (e->age > 1) {
            e->x += 11 * Q;
            e->vertical_speed += 3 * Q / 2;
            if (e->vertical_speed > 63 * Q / 2) e->vertical_speed = 63 * Q / 2;
            e->y += e->vertical_speed;
        }
        if (e->position[1] >= -3071) {
            place(e, e->marks[3]);
            e->action = e->yaw = e->vertical_speed = 0;
            enter(e, BFM_ESCAPE_REVEAL);
        }
        break;
    case BFM_ESCAPE_REVEAL:
        if (e->age >= 63) {
            e->checkpoint = BFM_ESCAPE_UPPER_CHECKPOINT;
            e->forward_speed = 8 * Q;
            enter(e, BFM_ESCAPE_UPPER);
        }
        break;
    case BFM_ESCAPE_UPPER:
    case BFM_ESCAPE_LOWER:
        run_downhill(e, field, horizontal, cross_edge, cross_held);
        break;
    case BFM_ESCAPE_DROP:
        e->z -= 16 * Q;
        e->y += 16 * Q;
        if (e->age >= 17) { e->airborne = 1; e->action = 3; enter(e, BFM_ESCAPE_LAND); }
        break;
    case BFM_ESCAPE_LAND: {
        if (e->airborne) {
            int result = ledge_vertical_step(e, field, e->y + 15 * Q);
            if (!result) return 0;
            if (result == 2) e->airborne = 0;
        }
        if (e->age >= 17) { e->action = 0; enter(e, BFM_ESCAPE_RECOVER); }
        break;
    }
    case BFM_ESCAPE_RECOVER:
        /* At the tower foot the event pulses Cross every sixteen updates.
         * The rock's landing and flash continue on its own actor clock. */
        if (e->age >= 50 && e->age <= 98 && (e->age - 50) % 16 == 0) {
            e->vertical_speed = -18 * Q;
            e->airborne = 1;
            e->action = 2;
        } else if (e->airborne) {
            e->vertical_speed += e->vertical_speed < 0 ? 21 * Q / 8 : 13 * Q / 8;
            int result = ledge_vertical_step(e, field, e->y + e->vertical_speed);
            if (!result) return 0;
            if (result == 2) {
                e->airborne = e->vertical_speed = e->action = 0;
            }
        }
        if (e->age >= 49 + 49 + 17 + 2) {
            e->checkpoint = BFM_ESCAPE_LOWER_CHECKPOINT;
            e->forward_speed = 9 * Q;
            enter(e, BFM_ESCAPE_LOWER);
        }
        break;
    case BFM_ESCAPE_CAUGHT:
        e->action = e->age < 7 ? 25 : e->age == 7 ? 0 : 26;
        break;
    default:
        break;
    }
    if (e->phase == BFM_ESCAPE_CAUGHT && !e->age) e->action = 25;
    e->position[0] = (int16_t)whole(e->x);
    e->position[1] = (int16_t)whole(e->y);
    e->position[2] = (int16_t)whole(e->z);
    if (!bfm_tower_escape_head_tick(&e->head, field, e->phase, e->age, e->position)) return 0;
    if (e->phase == BFM_ESCAPE_UPPER || e->phase == BFM_ESCAPE_LOWER) {
        if (e->head.caught) {
            enter(e, BFM_ESCAPE_CAUGHT);
            e->action = 25;
        } else if (e->phase == BFM_ESCAPE_LOWER && e->head.exit_ready && e->position[2] < -6480) {
            enter(e, BFM_ESCAPE_REQUEST);
        }
    }
    return 1;
}

int bfm_tower_escape_take_castle(BfmTowerEscape *e, RoomTransferDescriptor *destination)
{
    if (!e || e->phase != BFM_ESCAPE_REQUEST || !destination) return 0;
    *destination = e->destination;
    enter(e, BFM_ESCAPE_TRANSFERRED);
    return 1;
}
