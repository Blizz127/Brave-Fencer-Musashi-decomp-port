/* SPDX-License-Identifier: MIT */
#include "forest_boulders.h"
#include "forest_opening_data.h"
#include "forest_shore_motion.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>

int bfm_forest_boulders_load(BfmForestBoulders *rocks, const BfmForestField *field,
                             uint32_t placement_list)
{
    const uint8_t *velocity = bfm_forest_room_bytes(field, UINT32_C(0x8018915c), 12);
    const uint8_t *shape = bfm_forest_room_bytes(field, UINT32_C(0x8018a884), 16);
    if (!velocity || !shape || u32le(shape) || u32le(velocity) || u32le(velocity + 4))
        return 0;
    int64_t speed = (int64_t)u32le(velocity + 8) - INT64_C(0x100000000);
    if (speed >= 0 || speed < -16 * 65536) return 0;
    rocks->forward_speed = (int32_t)speed;
    for (unsigned axis = 0; axis < 3; ++axis) {
        rocks->box.low[axis] = s16le(shape + 4 + 4 * axis);
        rocks->box.high[axis] = s16le(shape + 6 + 4 * axis);
        if (rocks->box.low[axis] > rocks->box.high[axis]) return 0;
    }
    unsigned count = 0;
    for (unsigned i = 0; i < 64; ++i) {
        const uint8_t *row = bfm_forest_room_bytes(field, placement_list + i * 20, 20);
        if (!row) return 0;
        unsigned kind = u16le(row + 6);
        if (!kind) {
            if (count != BFM_FOREST_BOULDERS) return 0;
            rocks->art = bfm_forest_barrier_art_open(field);
            return rocks->art != NULL;
        }
        if (kind != 15 || u16le(row + 8)) continue;
        if (count == BFM_FOREST_BOULDERS) return 0;
        BfmForestBoulder *body = &rocks->placed[count++];
        for (unsigned axis = 0; axis < 3; ++axis) body->at[axis] = s16le(row + 2 * axis);
        body->yaw = u16le(row + 12);
        bfm_forest_actor_root(body->yaw, body->at, &body->root);
    }
    return 0;
}

void bfm_forest_boulders_tick(BfmForestBoulders *rocks, int16_t player_z)
{
    for (unsigned i = 0; i < BFM_FOREST_BOULDERS; ++i) {
        BfmForestBoulder *body = &rocks->placed[i];
        if (!body->active && abs((int)player_z - body->at[2]) < 768)
            body->active = 1;
    }
}

unsigned bfm_forest_boulders_draw(const BfmForestBoulders *rocks, BfmForestActor *actors)
{
    unsigned count = 0;
    for (unsigned i = 0; i < BFM_FOREST_BOULDERS; ++i) {
        const BfmForestBoulder *body = &rocks->placed[i];
        if (body->active && bfm_forest_barrier_art_actor(rocks->art, &body->root, &actors[count]))
            ++count;
    }
    return count;
}

/* The room rotates each boulder's solid box with its placed body. */
int bfm_forest_boulder_contact(const BfmForestBoulders *rocks, const int16_t from[3],
                               const int16_t to[3], int16_t resolved[3])
{
    int dx = to[0] - from[0], dz = to[2] - from[2];
    for (unsigned i = 0; i < BFM_FOREST_BOULDERS; ++i) {
        const BfmForestBoulder *body = &rocks->placed[i];
        if (body->active && (int64_t)dx * (body->at[0] - to[0]) + (int64_t)dz * (body->at[2] - to[2]) > 0 &&
            bfm_forest_body_slide(&rocks->box, &body->root, from, to, resolved))
            return (int)i + 1;
    }
    return 0;
}

/* The clearance query borrows the boxes without the model's scale or yaw.
 * The direct walking collision above still uses the rotated body. */
int bfm_forest_boulder_obstruction(const BfmForestGuardBox *box, const int16_t at[3],
                                   const int16_t from[3], const int16_t to[3])
{
    const BfmForestTransform root = {
        .rotation = {{4096, 0, 0}, {0, 4096, 0}, {0, 0, 4096}},
        .translation = {at[0], at[1], at[2]}
    };
    int16_t center[3];
    return bfm_forest_guard_contact(box, &root, from, to, center);
}

int bfm_forest_boulder_move(BfmForestField *field, unsigned placement,
                            const int16_t from[3], const int16_t to[3])
{
    BfmForestBoulders *rocks = bfm_forest_room_boulders(field);
    if (placement >= BFM_FOREST_BOULDERS || !rocks->placed[placement].active) return -1;
    BfmForestBoulder *body = &rocks->placed[placement];
    int dx = to[0] - from[0], dz = to[2] - from[2];
    int16_t touch[3];
    if (!(dx || dz) || !bfm_forest_body_slide(&rocks->box, &body->root, from, to, touch) ||
        (int64_t)dx * (body->at[0] - to[0]) + (int64_t)dz * (body->at[2] - to[2]) <= 0) return -1;

    /* The moving rock tests three leading rays against other solid actors. */
    unsigned heading = bfm_forest_shore_bearing(-dx, -dz);
    const int16_t *old = body->at;
    const int16_t probe_angles[3] = {0, -341, 341};
    for (unsigned ray = 0; ray < 3; ++ray) {
        BfmForestTransform root;
        int16_t tip[3];
        bfm_forest_actor_root(heading + probe_angles[ray], old, &root);
        for (unsigned axis = 0; axis < 3; ++axis) {
            int w = -128 * root.rotation[axis][2];
            tip[axis] = (int16_t)(old[axis] + w / 4096 - (w % 4096 < 0));
        }
        if (bfm_forest_boulder_path_blocked(field, placement, old, tip)) return 0;
    }
    int16_t next[3] = {(int16_t)(old[0] + dx), (int16_t)(old[1] + to[1] - from[1]),
                       (int16_t)(old[2] + dz)};
    BfmForestContact ground;
    if (!bfm_forest_ground(field, next[0], next[1] - 4, next[2], &ground) || ground.flags ||
        ground.normal[0] || ground.normal[2] || ground.height != old[1]) return 0;
    for (unsigned axis = 0; axis < 3; ++axis) body->at[axis] = next[axis];
    bfm_forest_actor_root(body->yaw, body->at, &body->root);
    return 1;
}
