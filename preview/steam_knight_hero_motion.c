/* SPDX-License-Identifier: MIT */
#include "steam_knight_hero_motion.h"
#include "forest_hurt.h"
#include "forest_motion.h"
#include "forest_walk.h"
#include "forest_action.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

enum { Q = 65536, STILL = 0, WALK = 7, SWING = 5, RECOIL = 25 };
struct BfmSteamKnightHeroMotion {
    BfmForestField *surface;
    ForestPlayerState actor;
    BfmForestManual ground;
    BfmForestJump jump;
    BfmForestJumpPoses jump_poses;
    BfmForestSwing swing;
    BfmForestHurtClip hurt;
    BfmSteamKnightHeroSample sample;
    int32_t start_speed;
    unsigned swing_tick, next_kind, hurt_side, hurt_tick;
    int16_t recoil_heading;
};

BfmSteamKnightHeroMotion *bfm_steam_hero_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, const int16_t feet[3], int16_t heading,
    int32_t initial_walk_velocity)
{
    const uint32_t base = UINT32_C(0x80128158), slot = UINT32_C(0x80190894);
    if (!disc || !scene || !feet || bytes < slot - base + 4 ||
        initial_walk_velocity > -Q || initial_walk_velocity < -11 * Q) return NULL;
    uint32_t room = u32le(scene + (slot - base));
    if (room < base || room - base > bytes || bytes - (room - base) < 0x8c) return NULL;
    uint32_t table = u32le(scene + (room - base) + 0x4c);
    BfmSteamKnightHeroMotion *h = calloc(1, sizeof *h);
    if (!h) return NULL;
    h->surface = bfm_forest_motion_surface_open(scene, bytes, room);
    BfmForestContact support;
    if (!h->surface || !bfm_forest_ground(h->surface, feet[0], feet[1], feet[2], &support) ||
        abs(support.height - feet[1]) > 4 || !bfm_forest_jump_surface(&support) ||
        !bfm_forest_jump_poses_load(disc, table, &h->jump_poses) ||
        !bfm_forest_swing_load(disc, &h->swing) || !bfm_forest_hurt_load(disc, &h->hurt)) {
        bfm_steam_hero_free(h);
        return NULL;
    }
    h->start_speed = h->ground.entry_velocity = initial_walk_velocity;
    memcpy(h->sample.feet, feet, sizeof h->sample.feet);
    for (unsigned axis = 0; axis < 3; ++axis)
        h->actor.position[axis] = (uint32_t)(uint16_t)feet[axis] << 16;
    h->sample.heading = heading;
    h->actor.phase = 1;
    return h;
}

void bfm_steam_hero_free(BfmSteamKnightHeroMotion *h)
{
    if (h) bfm_forest_field_free(h->surface);
    free(h);
}

const BfmSteamKnightHeroSample *bfm_steam_hero_sample(const BfmSteamKnightHeroMotion *h)
{
    return h ? &h->sample : NULL;
}

static void rest(BfmSteamKnightHeroMotion *h)
{
    memset(h->actor.velocity, 0, sizeof h->actor.velocity);
    h->actor.field_24.word = h->actor.local_y.word = h->actor.field_2c.word = 0;
    h->actor.state = 0;
    h->actor.phase = 1;
    h->jump = (BfmForestJump){0};
    h->ground = (BfmForestManual){.entry_velocity = h->start_speed};
    h->sample.body = h->sample.motion = h->sample.elapsed = 0;
}

static int jump_pose(BfmSteamKnightHeroMotion *h)
{
    enum BfmForestJumpPhase phase = h->jump.landed ? BFM_FOREST_JUMP_LANDING :
        h->jump.descending ? BFM_FOREST_JUMP_FALLING : BFM_FOREST_JUMP_RISING;
    h->sample.elapsed = h->jump.phase_elapsed;
    h->sample.body = bfm_forest_jump_pose_frame(&h->jump_poses, phase, h->jump.phase_elapsed);
    return h->sample.body != 0;
}

static int swing(BfmSteamKnightHeroMotion *h, int starting, BfmSteamKnightHeroStrike *strike)
{
    if (starting) {
        h->swing_tick = 0;
        strike->issued = 1;
        h->next_kind ^= 1;
        h->actor.state = SWING;
    } else {
        ++h->swing_tick;
    }
    const BfmForestSwingFrame *key = bfm_forest_swing_frame(&h->swing, h->swing_tick);
    if (!key) {
        /* The airborne slash suspends the preceding jump. Its next physical
         * update still owns the undischarged velocity and fractional feet. */
        if (h->jump.active && !h->jump.landed) {
            h->actor.state = h->jump.walk_off ? 3 : 2;
            return jump_pose(h);
        }
        rest(h);
        return 1;
    }
    strike->type = h->next_kind ? 1 : 2;
    strike->power_class = strike->type - 1;
    strike->roll_strength = !starting;
    h->sample.elapsed = h->swing_tick;
    if (key->window) {
        const BfmForestSwingFrame *next = bfm_forest_swing_frame(&h->swing, h->swing_tick + 1);
        if (!next) return 0;
        BfmForestTransform root;
        bfm_forest_actor_root(h->sample.heading, h->sample.feet, &root);
        strike->count = bfm_forest_blade_rays(&root, key->tip, next->tip, strike->from, strike->ends);
    }
    /* The existing field displays the blade poses two updates after its action keys. */
    if (h->swing_tick >= 2)
        h->sample.body = h->swing.frames[h->swing_tick - 2].pose;
    return 1;
}

int bfm_steam_hero_recoil(BfmSteamKnightHeroMotion *h, int16_t heading)
{
    if (!h || h->actor.state == RECOIL) return 0;
    BfmForestHurt direction = {0};
    if (!bfm_forest_hurt_begin(&direction, h->sample.heading, heading)) return 0;
    h->hurt_side = direction.side;
    h->hurt_tick = 0;
    h->jump = (BfmForestJump){0};
    h->recoil_heading = heading;
    h->actor.field_24.word = 0;
    h->actor.field_2c.word = -(11 * Q + 5 * Q / 16);
    h->actor.local_y.word = -31 * Q;
    h->actor.state = RECOIL;
    return 1;
}

static int recoil_step(BfmSteamKnightHeroMotion *h)
{
    /* The two witnessed airborne impacts launch the same vertical arc. */
    ForestPlayerState actor = h->actor;
    actor.local_y.word += 3 * Q / 2;
    if (actor.local_y.word > 31 * Q) actor.local_y.word = 31 * Q;
    if (!bfm_forest_level_velocity(&actor, 0, h->recoil_heading, 0)) return 0;
    func_801473EC(&actor);
    int16_t next[3];
    for (unsigned i = 0; i < 3; ++i) next[i] = (int16_t)(actor.position[i] >> 16);
    BfmForestContact floor;
    int landed;
    if (!bfm_forest_air_step(h->surface, h->sample.feet, next, &floor, &landed)) {
        next[0] = h->sample.feet[0];
        next[2] = h->sample.feet[2];
        if (!bfm_forest_air_step(h->surface, h->sample.feet, next, &floor, &landed)) return 0;
    }
    for (unsigned i = 0; i < 3; ++i)
        actor.position[i] = ((uint32_t)(uint16_t)next[i] << 16) | (actor.position[i] & 0xffffu);
    memcpy(h->sample.feet, next, sizeof next);
    h->actor = actor;
    unsigned key = h->hurt_tick < BFM_FOREST_HURT_KEYS ? h->hurt_tick : BFM_FOREST_HURT_KEYS - 1;
    h->sample.body = h->hurt.pose[h->hurt_side][key];
    h->sample.elapsed = h->hurt_tick++;
    if (landed) rest(h);
    return 1;
}

int bfm_steam_hero_tick(BfmSteamKnightHeroMotion *h, const BfmForestCamera *camera,
    int x, int y, uint16_t pressed, uint16_t held, BfmSteamKnightHeroStrike *strike)
{
    if (!h || !camera || !strike || x < -1 || x > 1 || y < -1 || y > 1) return 0;
    *strike = (BfmSteamKnightHeroStrike){0};
    int ok = 1;
    if (h->actor.state == RECOIL) {
        ok = recoil_step(h);
    } else if (h->actor.state == SWING) {
        ok = swing(h, 0, strike);
    } else if (!h->jump.active && (pressed & BFM_FOREST_CROSS) &&
        bfm_forest_jump_begin(&h->jump, h->surface, &h->actor, h->sample.feet)) {
        ok = jump_pose(h);
    } else if (pressed & BFM_FOREST_SQUARE) {
        ok = swing(h, 1, strike);
    } else if (h->jump.active) {
        BfmForestJumpResult result = bfm_forest_jump_tick(&h->jump, h->surface, &h->actor,
            camera, x, y, !!(held & BFM_FOREST_CROSS), &h->sample.heading, h->sample.feet);
        if (result == BFM_FOREST_JUMP_FINISHED) {
            bfm_forest_jump_resume(&h->ground, &h->jump, &h->actor);
            h->ground.entry_velocity = h->start_speed;
            h->sample.body = h->sample.elapsed = 0;
            h->sample.motion = h->actor.state ? WALK : STILL;
        } else if (result != BFM_FOREST_JUMP_BOUNDARY) {
            ok = jump_pose(h);
        }
    } else {
        BfmForestManualResult result = bfm_forest_manual_tick(&h->ground, h->surface, &h->actor,
            camera, x, y, &h->sample.heading, h->sample.feet);
        ok = result != BFM_FOREST_MANUAL_ERROR;
        h->sample.body = 0;
        h->sample.motion = h->ground.motion;
        h->sample.elapsed = h->ground.elapsed;
        if (result == BFM_FOREST_MANUAL_FALL) {
            if (!bfm_forest_fall_begin(&h->jump, &h->ground, &h->actor)) return 0;
            BfmForestJumpResult fall = bfm_forest_jump_tick(&h->jump, h->surface, &h->actor,
                camera, x, y, !!(held & BFM_FOREST_CROSS), &h->sample.heading, h->sample.feet);
            ok = fall != BFM_FOREST_JUMP_BOUNDARY && jump_pose(h);
        }
    }
    h->sample.action = h->actor.state;
    return ok;
}
