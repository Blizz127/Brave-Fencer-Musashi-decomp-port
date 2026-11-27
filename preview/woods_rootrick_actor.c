/* SPDX-License-Identifier: MIT */
#include "woods_rootrick_actor.h"
#include "field_waypoint.h"
#include "forest_pose.h"
#include "village_ground.h"
#include "../pc_port/forest_player_bytes.h"

#include <limits.h>
#include <string.h>

enum { Q16 = 65536, FALL_ACCEL = 98304 };
enum {
    STANDBY, FIRST_FALL, WAIT_FOR_SCORE, FIRST_LANDING, RISE_FROM_FIRST_LANDING,
    JUMP_TO_CLIFF, CLIMB_BRIEFLY, LEAVE_CLIFF, SECOND_FALL, SECOND_LANDING,
    RISE_FROM_SECOND_LANDING, START_RACE_JUMP, JUMP_TO_RACE_WALL, RACE_WALL
};

static const uint32_t standing = UINT32_C(0x801f3ea4);
static const uint32_t climbing = UINT32_C(0x801f3efc);
static const uint32_t rising = UINT32_C(0x801f404c);
static const uint32_t falling = UINT32_C(0x801f40a4);

int bfm_woods_rootrick_actor_begin(BfmWoodsRootrickActor *a, const BfmVillageGround *ground,
                                   const uint8_t *scene, size_t bytes, const int16_t xyz[3])
{
    const size_t header = 0x80190ce4u - 0x80128158u;
    const size_t jump = 0x80195114u - 0x80128158u, climb = 0x801950d8u - 0x80128158u;
    const size_t probe = 0x80195144u - 0x80128158u;
    if (!a || !ground || !scene || !xyz || bytes < probe + 6 ||
        u32le(scene + header) != UINT32_C(0x801961e8)) return 0;
    BfmWoodsRootrickActor next = {.ground = ground, .clip = standing, .gravity = FALL_ACCEL};
    memcpy(next.xyz, xyz, sizeof next.xyz);
    for (unsigned i = 0; i < 3; ++i) {
        next.jump[i] = (int32_t)u32le(scene + jump + 4*i);
        next.climb[i] = (int32_t)u32le(scene + climb + 4*i);
        next.probe[i] = s16le(scene + probe + 2*i);
    }
    *a = next;
    return 1;
}

void bfm_woods_rootrick_actor_bind_clip(BfmWoodsRootrickActor *a, uint32_t clip)
{
    a->clip = clip;
    a->elapsed = 0;
    a->pose_held = 0;
}

void bfm_woods_rootrick_actor_set_yaw(BfmWoodsRootrickActor *a, unsigned yaw)
{
    a->yaw = yaw & 4095;
}

void bfm_woods_rootrick_actor_turn_to(BfmWoodsRootrickActor *a, const int16_t player[3])
{
    /* The model faces along its negative Z axis. The script takes this
     * bearing once and gives the model thirty updates to finish turning. */
    a->target_yaw = (unsigned)bfm_waypoint_heading(a->xyz[0]-player[0], a->xyz[2]-player[2]);
    a->turn_fields = 30;
}

int bfm_woods_rootrick_actor_stage(BfmWoodsRootrickActor *a, unsigned script_stage)
{
    if (!a || !a->ground) return 0;
    if (script_stage == 0 && a->stage == STANDBY) a->stage = FIRST_FALL;
    else if (script_stage == 5 && a->stage == CLIMB_BRIEFLY) a->stage = LEAVE_CLIFF;
    else return 0;
    return 1;
}

void bfm_woods_rootrick_actor_begin_wall(BfmWoodsRootrickActor *a)
{
    a->stage = START_RACE_JUMP;
}

void bfm_woods_rootrick_actor_step_turn(BfmWoodsRootrickActor *a)
{
    if (!a->turn_fields) return;
    if (!--a->turn_fields) {
        a->yaw = a->target_yaw;
        return;
    }
    int delta = (int)a->target_yaw - (int)a->yaw;
    if (delta > 2048) delta -= 4096;
    if (delta < -2048) delta += 4096;
    int step = delta / 6;
    if (!step) step = (delta > 0) - (delta < 0);
    a->yaw = (unsigned)((int)a->yaw + step) & 4095;
}

void bfm_woods_rootrick_actor_impulse(BfmWoodsRootrickActor *a, const int32_t vector[3],
                                     int32_t gravity)
{
    BfmForestTransform model;
    bfm_forest_actor_pitch_yaw((unsigned)a->pitch, a->yaw, a->xyz, &model);
    for (unsigned i = 0; i < 3; ++i) {
        int64_t dot = 0;
        for (unsigned k = 0; k < 3; ++k) dot += (int64_t)model.rotation[i][k] * vector[k];
        a->velocity[i] = (int32_t)(dot / 4096 - (dot % 4096 < 0));
    }
    a->gravity = gravity;
}

static int move(BfmWoodsRootrickActor *a, const int16_t probe[3], int walking,
                unsigned *contact)
{
    if (!a || !a->ground || !contact) return 0;
    int16_t from[3], to[3];
    uint16_t fraction[3];
    int64_t vy = (int64_t)a->velocity[1] + a->gravity;
    if (vy < INT32_MIN || vy > INT32_MAX) return 0;
    for (unsigned i = 0; i < 3; ++i) {
        int64_t endpoint = (int64_t)a->xyz[i]*Q16 + a->fraction[i] + (i == 1 ? vy : a->velocity[i]);
        int64_t whole = endpoint / Q16 - (endpoint % Q16 < 0);
        int offset = probe ? probe[i] : 0;
        int start = a->xyz[i] + offset;
        if (start < INT16_MIN || start > INT16_MAX || whole + offset < INT16_MIN ||
            whole + offset > INT16_MAX) return 0;
        from[i] = (int16_t)start;
        to[i] = (int16_t)(whole + offset);
        fraction[i] = (uint16_t)endpoint;
    }
    *contact = 0;
    if (probe) {
        uint16_t material;
        *contact = bfm_village_ground_support_probe(a->ground, !walking, from, to, &material);
        if (walking && *contact) {
            int16_t support[3];
            memcpy(support, to, sizeof support);
            unsigned surface = bfm_village_ground_support_probe(a->ground, 2, to, support, &material);
            if (surface & 0x6000) {
                to[1] = support[1];
                *contact = (surface & 0x6000) | (*contact & 0x8000);
            } else {
                *contact = 0;
            }
        }
    }
    int16_t xyz[3];
    for (unsigned i = 0; i < 3; ++i) {
        int result = to[i] - (probe ? probe[i] : 0);
        if (result < INT16_MIN || result > INT16_MAX) return 0;
        xyz[i] = (int16_t)result;
    }
    memcpy(a->xyz, xyz, sizeof xyz);
    memcpy(a->fraction, fraction, sizeof fraction);
    a->velocity[1] = (*contact & 0x6000) ? 0 : (int32_t)vy;
    return 1;
}

int bfm_woods_rootrick_actor_move(BfmWoodsRootrickActor *a, const int16_t probe[3],
                                 unsigned *contact)
{
    return move(a, probe, 0, contact);
}

int bfm_woods_rootrick_actor_walk(BfmWoodsRootrickActor *a, const int16_t probe[3],
                                 unsigned *contact)
{
    return probe && move(a, probe, 1, contact);
}

int bfm_woods_rootrick_actor_step(BfmWoodsRootrickActor *a, int score_requested)
{
    static const int16_t feet[3];
    if (!a || !a->ground) return BFM_ROOTRICK_ACTOR_ERROR;
    int events = 0;
    unsigned contact;
    if (!a->pose_held && a->elapsed != UINT_MAX) ++a->elapsed;
    switch (a->stage) {
    case FIRST_FALL:
    case SECOND_FALL:
        a->pitch -= 32;
        if (a->pitch < -1024) a->pitch = -1024;
        if (!bfm_woods_rootrick_actor_move(a, feet, &contact)) return BFM_ROOTRICK_ACTOR_ERROR;
        if (contact & 0x2000) {
            events = BFM_ROOTRICK_ACTOR_IMPACT;
            if (a->stage == FIRST_FALL) {
                a->stage = WAIT_FOR_SCORE;
                events |= BFM_ROOTRICK_ACTOR_RELEASE;
            } else {
                a->pitch = -1024;
                a->timer = 45;
                a->stage = SECOND_LANDING;
            }
        }
        break;
    case WAIT_FOR_SCORE:
        if (!score_requested) {
            a->pitch = -1024;
            a->timer = 45;
            a->stage = FIRST_LANDING;
            events = BFM_ROOTRICK_ACTOR_RELEASE;
        }
        break;
    case FIRST_LANDING:
    case SECOND_LANDING:
        if (!--a->timer)
            a->stage = a->stage == FIRST_LANDING ? RISE_FROM_FIRST_LANDING : RISE_FROM_SECOND_LANDING;
        break;
    case RISE_FROM_FIRST_LANDING:
    case RISE_FROM_SECOND_LANDING:
        a->pitch += 128;
        if (a->pitch >= 0) {
            a->pitch = 0;
            if (a->stage == RISE_FROM_FIRST_LANDING) {
                bfm_woods_rootrick_actor_impulse(a, a->jump, FALL_ACCEL);
                bfm_woods_rootrick_actor_bind_clip(a, rising);
                a->timer = 10;
                a->stage = JUMP_TO_CLIFF;
            } else {
                bfm_woods_rootrick_actor_bind_clip(a, standing);
                a->stage = STANDBY;
                events = BFM_ROOTRICK_ACTOR_RELEASE;
            }
        }
        break;
    case JUMP_TO_CLIFF:
    case JUMP_TO_RACE_WALL:
        if (a->timer && --a->timer) break;
        if (!bfm_woods_rootrick_actor_move(a, a->probe, &contact)) return BFM_ROOTRICK_ACTOR_ERROR;
        if (contact & 0x8000) {
            if (a->stage == JUMP_TO_CLIFF) {
                bfm_woods_rootrick_actor_impulse(a, a->climb, 0);
                bfm_woods_rootrick_actor_bind_clip(a, climbing);
                a->timer = 30;
                a->stage = CLIMB_BRIEFLY;
                events = BFM_ROOTRICK_ACTOR_RELEASE;
            } else {
                a->pose_held = 1;
                a->stage = RACE_WALL;
                events = BFM_ROOTRICK_ACTOR_AT_WALL;
            }
        } else if (a->stage == JUMP_TO_RACE_WALL) {
            if (contact & 0x6000) a->stage = START_RACE_JUMP;
            else if (a->velocity[1] >= 0 && a->clip == rising)
                bfm_woods_rootrick_actor_bind_clip(a, falling);
        }
        break;
    case CLIMB_BRIEFLY:
        if (a->timer && --a->timer && !bfm_woods_rootrick_actor_move(a, NULL, &contact))
            return BFM_ROOTRICK_ACTOR_ERROR;
        break;
    case LEAVE_CLIFF:
        memset(a->velocity, 0, sizeof a->velocity);
        a->gravity = FALL_ACCEL;
        a->pose_held = 1;
        a->stage = SECOND_FALL;
        break;
    case START_RACE_JUMP:
        bfm_woods_rootrick_actor_impulse(a, a->jump, FALL_ACCEL);
        bfm_woods_rootrick_actor_bind_clip(a, rising);
        a->timer = 10;
        a->stage = JUMP_TO_RACE_WALL;
        events = BFM_ROOTRICK_ACTOR_START_WALL;
        break;
    case STANDBY:
    case RACE_WALL:
        break;
    default:
        return BFM_ROOTRICK_ACTOR_ERROR;
    }
    bfm_woods_rootrick_actor_step_turn(a);
    return events;
}
