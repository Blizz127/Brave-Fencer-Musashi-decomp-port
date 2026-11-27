/* SPDX-License-Identifier: MIT */
#include "woods_rootrick_boulder.h"
#include "field_waypoint.h"
#include "forest_lift.h"
#include "woods_rootrick_actor.h"
#include "../pc_port/forest_player_bytes.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

enum BoulderPhase { HELD_BY_RIVAL, HELD_BY_HERO, WINDUP, FLIGHT_TO_HERO,
                    FLIGHT_TO_RIVAL, BLINK, REMOVED, FAILED };
enum { WINDUP_KEYS = 16, GRAVITY_Q16 = 98304 };
static const uint32_t base = UINT32_C(0x80128158);

struct BfmWoodsRootrickBoulder {
    MusashiForestPlayerMesh *mesh;
    BfmWoodsRootrickActor motion;
    enum BoulderPhase phase;
    int16_t grip[3], segment[2][3];
    int32_t throw_velocity[3];
    BfmForestGuardBox body[2];
    const uint8_t *rest_pose, *windup[WINDUP_KEYS], *display_pose;
    unsigned key, timer, queued_throw, struck, hidden;
};

static const uint8_t *span(const uint8_t *scene, size_t bytes, uint32_t address, size_t len)
{
    if (address < base || address - base > bytes || len > bytes - (address - base)) return NULL;
    return scene + (address - base);
}

int bfm_woods_rootrick_boulder_target_boxes(const uint8_t *scene, size_t bytes,
    BfmForestGuardBox out[2])
{
    if (!scene || !out) return 0;
    const uint8_t *profile = span(scene, bytes, UINT32_C(0x801950a4), 12);
    if (!profile) return 0;
    BfmForestGuardBox decoded[2];
    const uint8_t *boxes[2] = {
        span(scene, bytes, u32le(profile + 8), 16),
        span(scene, bytes, UINT32_C(0x801926e0), 16)
    };
    for (unsigned n = 0; n < 2; ++n) {
        if (!boxes[n] || u32le(boxes[n])) return 0;
        for (unsigned i = 0; i < 3; ++i) {
            decoded[n].low[i] = s16le(boxes[n] + 4 + 4 * i);
            decoded[n].high[i] = s16le(boxes[n] + 6 + 4 * i);
            if (decoded[n].low[i] > decoded[n].high[i]) return 0;
        }
    }
    memcpy(out, decoded, sizeof decoded);
    return 1;
}

BfmWoodsRootrickBoulder *bfm_woods_rootrick_boulder_spawn(const uint8_t *scene, size_t bytes,
    const BfmVillageGround *ground, BfmBoulderThrower thrower,
    const int16_t xyz[3], const uint16_t fractions[3])
{
    if (!scene || !ground || !xyz || (thrower != BFM_BOULDER_MUSASHI_THROWS &&
                                    thrower != BFM_BOULDER_ROOTRICK_THROWS)) return NULL;
    const uint8_t *room = span(scene, bytes, UINT32_C(0x80190ce4), 4);
    const uint8_t *poses = span(scene, bytes, UINT32_C(0x80195734), 152);
    const uint8_t *grip = span(scene, bytes, UINT32_C(0x8019454c), 6);
    const uint8_t *segment = span(scene, bytes, UINT32_C(0x801957cc) + 16 * (unsigned)thrower, 14);
    const uint8_t *velocity = span(scene, bytes, UINT32_C(0x801957ec), 12);
    if (!room || u32le(room) != UINT32_C(0x801961e8) || !poses || !grip ||
        !segment || !velocity) return NULL;
    BfmWoodsRootrickBoulder *rock = calloc(1, sizeof *rock);
    if (!rock) return NULL;
    unsigned parts;
    rock->mesh = musashi_forest_player_mesh_from_table(scene, bytes, base, UINT32_C(0x801ca540), &parts);
    if (!rock->mesh || parts != 1) goto fail;
    rock->rest_pose = span(scene, bytes, u32le(poses), 12);
    if (!rock->rest_pose || u32le(poses + 4) != 1 || u32le(poses + 8) ||
        u32le(poses + 12) != 0xffff) goto fail;
    for (unsigned k = 0; k < WINDUP_KEYS; ++k) {
        const uint8_t *entry = poses + 16 + 8 * k;
        rock->windup[k] = span(scene, bytes, u32le(entry), 12);
        if (!rock->windup[k] || u32le(entry + 4) != 1) goto fail;
    }
    if (u32le(poses + 144) || u32le(poses + 148) != 0xffff) goto fail;
    if (!bfm_woods_rootrick_boulder_target_boxes(scene, bytes, rock->body)) goto fail;
    for (unsigned i = 0; i < 3; ++i) {
        rock->grip[i] = s16le(grip + 2 * i);
        rock->throw_velocity[i] = (int32_t)u32le(velocity + 4 * i);
        rock->segment[0][i] = s16le(segment + 2 * i);
        rock->segment[1][i] = s16le(segment + 8 + 2 * i);
    }
    rock->motion.ground = ground;
    memcpy(rock->motion.xyz, xyz, sizeof rock->motion.xyz);
    if (thrower == BFM_BOULDER_MUSASHI_THROWS) {
        if (fractions) memcpy(rock->motion.fraction, fractions, sizeof rock->motion.fraction);
        rock->phase = HELD_BY_HERO;
    } else {
        rock->display_pose = rock->rest_pose;
    }
    return rock;
fail:
    bfm_woods_rootrick_boulder_free(rock);
    return NULL;
}

void bfm_woods_rootrick_boulder_free(BfmWoodsRootrickBoulder *rock)
{
    if (rock) {
        musashi_forest_player_mesh_free(rock->mesh);
        free(rock);
    }
}

int bfm_woods_rootrick_boulder_carry(BfmWoodsRootrickBoulder *rock,
    const int16_t socket[3], int held_yaw, int thrown)
{
    if (!rock || !socket || rock->phase != HELD_BY_HERO || rock->queued_throw) return 0;
    memcpy(rock->motion.xyz, socket, sizeof rock->motion.xyz);
    if (held_yaw >= 0) rock->motion.yaw = (unsigned)held_yaw & 4095;
    rock->queued_throw = thrown != 0;
    return 1;
}

const int16_t *bfm_woods_rootrick_boulder_grip(const BfmWoodsRootrickBoulder *rock)
{
    return rock ? rock->grip : NULL;
}

const BfmForestGuardBox *bfm_woods_rootrick_boulder_body(const BfmWoodsRootrickBoulder *rock,
    BfmBoulderThrower actor)
{
    if (!rock || (actor != BFM_BOULDER_ROOTRICK_THROWS && actor != BFM_BOULDER_MUSASHI_THROWS))
        return NULL;
    return &rock->body[actor];
}

const int16_t *bfm_woods_rootrick_boulder_xyz(const BfmWoodsRootrickBoulder *rock)
{
    return rock && rock->phase != REMOVED ? rock->motion.xyz : NULL;
}

static void aim(BfmWoodsRootrickBoulder *rock, const int16_t target[3])
{
    BfmWoodsRootrickActor *body = &rock->motion;
    int dy = target[1] - body->xyz[1];
    if (rock->struck || dy < 0) return;
    body->pitch = -bfm_waypoint_heading(target[2] - body->xyz[2], dy);
    bfm_woods_rootrick_actor_impulse(body, rock->throw_velocity, GRAVITY_Q16);
}

static BfmBoulderEvent fly(BfmWoodsRootrickBoulder *rock, const BfmBoulderTarget *target,
                            BfmBoulderImpact *impact)
{
    static const int16_t foot[3] = {0, 0, 0};
    if (!target || !target->foot || !target->model || !target->bounds) return BFM_BOULDER_ERROR;
    aim(rock, target->foot);
    BfmWoodsRootrickActor *body = &rock->motion;
    BfmForestTransform model;
    bfm_forest_actor_pitch_yaw((unsigned)body->pitch, body->yaw, body->xyz, &model);
    unsigned contact;
    if (!bfm_woods_rootrick_actor_move(body, foot, &contact)) return BFM_BOULDER_ERROR;
    if (contact & 0x6000) {
        rock->phase = BLINK;
        rock->timer = 30;
        return BFM_BOULDER_QUIET;
    }
    if (rock->struck) return BFM_BOULDER_QUIET;
    int16_t start[3], end[3], center[3];
    /* The probes use the last published model matrix, from before the ground
     * sweep. The model's new position is presented at the end of this field. */
    bfm_forest_lift_world(&model, rock->segment[0], start);
    bfm_forest_lift_world(&model, rock->segment[1], end);
    if (!bfm_forest_guard_contact_point(target->bounds, target->model, start, end, center,
                                        impact->impact, impact->normal))
        return BFM_BOULDER_QUIET;
    rock->struck = 1;
    if (rock->phase == FLIGHT_TO_HERO) {
        impact->voice = 0x4292;
        impact->hit_flags = 0xc001;
        impact->power = 1000;
        impact->hero_z_step = 32;
        impact->block_revival = 1;
        return BFM_BOULDER_STRUCK_MUSASHI;
    }
    impact->voice = 0x4331;
    return BFM_BOULDER_STRUCK_ROOTRICK;
}

BfmBoulderEvent bfm_woods_rootrick_boulder_step(BfmWoodsRootrickBoulder *rock,
    const BfmBoulderTarget *hero, const BfmBoulderTarget *rival, unsigned rival_yaw,
    uint32_t rival_clip, BfmBoulderImpact *impact)
{
    if (!rock || !impact) return BFM_BOULDER_ERROR;
    *impact = (BfmBoulderImpact){0};
    BfmWoodsRootrickActor *body = &rock->motion;
    switch (rock->phase) {
    case HELD_BY_RIVAL:
        if (!rival || !rival->foot) return BFM_BOULDER_ERROR;
        body->xyz[0] = rival->foot[0];
        body->xyz[2] = rival->foot[2];
        body->yaw = rival_yaw & 4095;
        if (rival_clip == UINT32_C(0x801f432c)) {
            rock->phase = WINDUP;
            rock->timer = 14;
            rock->display_pose = rock->windup[0];
        }
        break;
    case HELD_BY_HERO:
        if (rock->queued_throw) {
            if (!rival || !rival->foot) return BFM_BOULDER_ERROR;
            body->gravity = GRAVITY_Q16;
            aim(rock, rival->foot);
            rock->phase = FLIGHT_TO_RIVAL;
        }
        break;
    case WINDUP:
        if (!hero || !hero->foot) return BFM_BOULDER_ERROR;
        /* The generic pose pass precedes this actor. Launch freezes key 14. */
        rock->display_pose = rock->windup[++rock->key];
        if (--rock->timer == 0) {
            int z = body->xyz[2] + 104;
            if (z > INT16_MAX) {
                rock->phase = FAILED;
                return BFM_BOULDER_ERROR;
            }
            body->xyz[2] = (int16_t)z;
            body->gravity = GRAVITY_Q16;
            aim(rock, hero->foot);
            unsigned ignored;
            if (!bfm_woods_rootrick_actor_move(body, NULL, &ignored)) {
                rock->phase = FAILED;
                return BFM_BOULDER_ERROR;
            }
            rock->phase = FLIGHT_TO_HERO;
        }
        break;
    case FLIGHT_TO_HERO: return fly(rock, hero, impact);
    case FLIGHT_TO_RIVAL: return fly(rock, rival, impact);
    case BLINK:
        rock->hidden = !rock->hidden;
        if (--rock->timer) break;
        rock->phase = REMOVED;
        return BFM_BOULDER_GONE;
    case REMOVED: return BFM_BOULDER_QUIET;
    case FAILED: return BFM_BOULDER_ERROR;
    }
    return BFM_BOULDER_QUIET;
}

int bfm_woods_rootrick_boulder_add(const BfmWoodsRootrickBoulder *rock,
    BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    if (!rock || !figure || !view) return 0;
    if (rock->phase == FAILED) return 0;
    if (rock->hidden || rock->phase == REMOVED) return 1;
    BfmForestPose pose = {0};
    const BfmWoodsRootrickActor *body = &rock->motion;
    BfmForestTransform root;
    bfm_forest_actor_pitch_yaw((unsigned)body->pitch, body->yaw, body->xyz, &root);
    if (rock->display_pose) {
        if (!bfm_forest_actor_pose_decode_full(rock->display_pose, 12, 1, &pose)) return 0;
    } else {
        static const int16_t zero[3] = {0, 0, 0};
        bfm_forest_actor_root(0, zero, &pose.local[0]);
    }
    BfmForestActor actor = {.mesh = rock->mesh, .pose = &pose, .root = &root, .visible_parts = 1};
    return bfm_bedroom_figure_add_actor(figure, view, &actor, NULL, NULL, 0);
}
