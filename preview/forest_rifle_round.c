/* SPDX-License-Identifier: MIT */
#include "forest_rifle_round.h"
#include "forest_rifle_wall.h"
#include "forest_guard_clip.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>
#include <stdlib.h>

enum { RIFLE_ROUND_KEYS = 16 };

struct BfmForestRifleRoundArt {
    MusashiForestPlayerMesh *mesh;
    BfmForestPose poses[RIFLE_ROUND_KEYS];
    int32_t local_speed[3];
};

static int64_t down(int64_t n, int divisor)
{
    return n / divisor - (n % divisor < 0);
}

BfmForestRifleRoundArt *bfm_forest_rifle_round_art_open(const BfmForestField *field)
{
    const uint8_t *speed = bfm_forest_room_bytes(field, UINT32_C(0x801b1290), 12);
    if (!speed) return NULL;
    BfmForestRifleRoundArt *art = calloc(1, sizeof *art);
    if (!art) return NULL;
    art->mesh = bfm_forest_rifle_mesh(field, UINT32_C(0x801b1430), 1);
    BfmForestGuardClip clip;
    if (!art->mesh ||
        !bfm_forest_guard_clip_read(field, UINT32_C(0x801b1208), 65535, &clip) ||
        clip.count != RIFLE_ROUND_KEYS) {
        bfm_forest_rifle_round_art_free(art);
        return NULL;
    }
    for (unsigned key = 0; key < RIFLE_ROUND_KEYS; ++key) {
        if (!bfm_forest_actor_pose_decode(clip.pose[key], 8, 1, &art->poses[key])) {
            bfm_forest_rifle_round_art_free(art);
            return NULL;
        }
    }
    for (unsigned axis = 0; axis < 3; ++axis) {
        uint32_t value = u32le(speed + 4 * axis);
        art->local_speed[axis] = (int32_t)((int64_t)(value ^ UINT32_C(0x80000000)) -
                                          INT64_C(0x80000000));
    }
    return art;
}

void bfm_forest_rifle_round_art_free(BfmForestRifleRoundArt *art)
{
    if (art) {
        musashi_forest_player_mesh_free(art->mesh);
        free(art);
    }
}

int bfm_forest_rifle_round_blocked(const BfmForestRifleRound *round,
                                   const BfmForestField *field)
{
    int16_t from[3], to[3];
    for (unsigned axis = 0; axis < 3; ++axis) {
        int64_t before = down(round->position_q16[axis], 65536);
        int64_t after = down(round->position_q16[axis] + round->velocity_q16[axis], 65536);
        if (before < INT16_MIN || before > INT16_MAX || after < INT16_MIN || after > INT16_MAX)
            return 1;
        from[axis] = (int16_t)before;
        to[axis] = (int16_t)after;
    }
    return bfm_forest_rifle_wall(field, from, to, NULL) != 0;
}

void bfm_forest_rifle_round_tick(BfmForestRifleRound *round,
                                 const BfmForestRifleRoundArt *art, int hit)
{
    if (!round || !art || !round->active) return;
    if (hit || ++round->age >= 100) {
        round->active = 0;
        return;
    }
    for (unsigned axis = 0; axis < 3; ++axis) {
        round->position_q16[axis] += round->velocity_q16[axis];
        if (round->age > 1)
            round->root.translation[axis] = (int32_t)down(round->position_q16[axis], 65536);
    }
}

int bfm_forest_rifle_round_issue(BfmForestRifleRound *round,
                                 const BfmForestRifleRoundArt *art, const BfmForestRifleShot *shot)
{
    if (!round || !art || !shot || round->active) return 0;
    BfmForestRifleRound next = {.active = 1, .yaw = shot->yaw};
    bfm_forest_actor_root((unsigned)shot->yaw, shot->muzzle, &next.root);
    for (unsigned axis = 0; axis < 3; ++axis) {
        next.position_q16[axis] = (int64_t)shot->muzzle[axis] * 65536;
        int64_t dot = 0;
        for (unsigned k = 0; k < 3; ++k)
            dot += (int64_t)next.root.rotation[axis][k] * art->local_speed[k];
        int64_t step = down(dot, 4096);
        if (step < INT32_MIN || step > INT32_MAX) return 0;
        next.velocity_q16[axis] = (int32_t)step;
    }
    bfm_forest_rifle_round_tick(&next, art, 0);
    *round = next;
    return 1;
}

int bfm_forest_rifle_round_actor(const BfmForestRifleRound *round,
                                 const BfmForestRifleRoundArt *art, BfmForestActor *actor)
{
    if (!round || !round->active || !art || !actor) return 0;
    unsigned key = (round->age - 1) % RIFLE_ROUND_KEYS;
    *actor = (BfmForestActor){.mesh = art->mesh, .root = &round->root, .pose = &art->poses[key],
                             .visible_parts = 1};
    return 1;
}
