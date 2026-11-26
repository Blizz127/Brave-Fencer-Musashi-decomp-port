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
    BfmForestCast charge_poses;
    BfmForestHurtClip hurt;
    BfmForestHurt mace_reaction;
    uint16_t mace_damage;
    int mace_recovered;
    unsigned mace_blink_history;
    const BfmForestLiftData *lift_data;
    BfmForestLift lift;
    BfmLiftStep lift_step;
    int16_t held_position[3];
    int lift_start;
    unsigned grip, charge_key, charge_moving, spin_age;
    int16_t spin_heading;
    unsigned braced, previous_brace, brace_age;
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
        !bfm_forest_cast_load(&h->charge_poses, disc, table) ||
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

uint32_t bfm_steam_hero_flags(const BfmSteamKnightHeroMotion *h)
{
    return h ? h->actor.flags : 0;
}

const BfmForestField *bfm_steam_hero_surface(const BfmSteamKnightHeroMotion *h)
{
    return h ? h->surface : NULL;
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
    h->braced = h->previous_brace = 0;
}

BfmSteamCornerStep bfm_steam_hero_corner(BfmSteamKnightHeroMotion *h,
                                         BfmSteamCorner *corner, int view_ready)
{
    if (!h || !corner) return BFM_CORNER_BLOCKED;
    if (corner->phase == BFM_CORNER_ENTRY) rest(h);
    /* The walk ends before the field starts turning. */
    BfmSteamCornerStep step = bfm_steam_corner_step(corner, h->surface, &h->actor,
        h->sample.feet, &h->sample.heading, 0, view_ready);
    if (step != BFM_CORNER_CONTROLS && step != BFM_CORNER_BLOCKED) {
        if (step == BFM_CORNER_WALK && h->sample.motion == WALK) ++h->sample.elapsed;
        else h->sample.elapsed = 0;
        h->sample.motion = step == BFM_CORNER_WALK ? WALK : STILL;
        h->sample.body = 0;
        h->sample.action = h->actor.state;
    }
    return step;
}

int bfm_steam_hero_place(BfmSteamKnightHeroMotion *h, const int16_t feet[3], int16_t heading)
{
    BfmForestContact support;
    if (!h || !feet || !bfm_forest_ground(h->surface, feet[0], feet[1], feet[2], &support) ||
        abs(support.height - feet[1]) > 4 || !bfm_forest_jump_surface(&support)) return 0;
    if (h->lift_data && (h->lift.clip != BFM_LIFT_THROW ||
        h->lift.age < h->lift_data->motion[BFM_LIFT_THROW].event)) return 0;
    rest(h);
    h->lift_data = NULL;
    h->lift = (BfmForestLift){0};
    h->lift_start = 0;
    h->lift_step = BFM_LIFT_PLAY;
    h->sample.action = 0;
    h->sample.heading = heading;
    for (unsigned i = 0; i < 3; ++i) {
        h->sample.feet[i] = feet[i];
        h->actor.position[i] = (uint32_t)(uint16_t)feet[i] << 16;
    }
    return 1;
}

static const uint8_t *scene_row(const uint8_t *scene, size_t bytes, uint32_t address,
                                size_t size)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (!scene || address < base || address - base > bytes || size > bytes - (address - base))
        return NULL;
    return scene + (address - base);
}

int bfm_steam_hero_lift_load(const MusashiDiscMedia *disc, const uint8_t *scene,
                            size_t bytes, BfmForestLiftData *out)
{
    BfmForestLiftData data;
    const uint8_t *room = scene_row(scene, bytes, UINT32_C(0x80190894), 4);
    if (!out || !room) return 0;
    const uint8_t *desc = scene_row(scene, bytes, u32le(room), 0x50);
    if (!desc || !bfm_forest_lift_motions(disc, u32le(desc + 0x4c), &data)) return 0;
    data.raise_offsets = scene_row(scene, bytes, UINT32_C(0x801931c0),
                                   3 * (data.motion[BFM_LIFT_RAISE].count - 1));
    data.toss_offsets = scene_row(scene, bytes, UINT32_C(0x801931fc),
                                  3 * data.motion[BFM_LIFT_THROW].event);
    const uint8_t *speed = scene_row(scene, bytes, UINT32_C(0x80192f48), 4);
    if (!data.raise_offsets || !data.toss_offsets || !speed) return 0;
    data.walking_speed = (int32_t)u32le(speed);
    if (data.walking_speed >= 0) return 0;
    *out = data;
    return 1;
}

int bfm_steam_hero_brace(BfmSteamKnightHeroMotion *h, const int16_t corrected_feet[3],
                         const BfmForestLiftData *data)
{
    if (!h) return 0;
    if (!corrected_feet) {
        h->braced = h->previous_brace = 0;
        return 1;
    }
    if (!data || h->actor.state != 1 || h->jump.active || h->lift_data) return 0;
    const BfmLiftMotion *clip = &data->motion[BFM_LIFT_BRACE];
    if (!clip->count || clip->count > BFM_LIFT_KEYS) return 0;
    if (!h->braced)
        h->brace_age = h->previous_brace ? (h->brace_age + 1) % clip->count : 0;
    h->braced = 1;
    for (unsigned axis = 0; axis < 3; axis += 2) {
        h->sample.feet[axis] = corrected_feet[axis];
        h->actor.position[axis] = ((uint32_t)(uint16_t)corrected_feet[axis] << 16) |
                                  (h->actor.position[axis] & 0xffffu);
    }
    h->sample.elapsed = h->brace_age;
    h->sample.body = clip->frames[h->brace_age];
    return 1;
}

static int grab_ready(const BfmSteamKnightHeroMotion *h, uint16_t pressed)
{
    return h && h->braced && h->actor.state == 1 && !h->jump.active &&
           (pressed & BFM_FOREST_SQUARE) && !(pressed & BFM_FOREST_CROSS);
}

int bfm_steam_hero_grab_segment(const BfmSteamKnightHeroMotion *h, uint16_t pressed,
                                int16_t from[3], int16_t to[3])
{
    if (!from || !to || !grab_ready(h, pressed)) return 0;
    const int16_t reach[3] = {0, -16, -32};
    BfmForestTransform root;
    bfm_forest_actor_root(h->sample.heading, h->sample.feet, &root);
    memcpy(from, h->sample.feet, sizeof h->sample.feet);
    bfm_forest_lift_world(&root, reach, to);
    return 1;
}

int bfm_steam_hero_grab(BfmSteamKnightHeroMotion *h, uint16_t pressed,
                        const BfmForestLiftData *data, const int16_t local_grip[3])
{
    if (!grab_ready(h, pressed) || !data || !local_grip) return 0;
    h->lift_data = data;
    h->lift = (BfmForestLift){.clip = BFM_LIFT_RAISE, .target = 1};
    memcpy(h->lift.grip, local_grip, sizeof h->lift.grip);
    rest(h);
    h->actor.state = 13;
    h->lift_start = 1;
    return 1;
}

BfmLiftStep bfm_steam_hero_carry(const BfmSteamKnightHeroMotion *h, int16_t world[3])
{
    if (!h) return BFM_LIFT_PLAY;
    if (world && (h->lift_step == BFM_LIFT_ATTACH || h->lift_step == BFM_LIFT_WALK_ATTACH))
        memcpy(world, h->held_position, sizeof h->held_position);
    return h->lift_step;
}

static int carry(BfmSteamKnightHeroMotion *h, const BfmForestCamera *camera,
                 int x, int y, uint16_t pressed)
{
    const BfmForestLiftData *data = h->lift_data;
    if (h->lift_start) {
        h->lift_start = 0;
    } else {
        h->lift_step = bfm_forest_lift_tick(&h->lift, data, x || y,
                                           !!(pressed & BFM_FOREST_SQUARE));
    }
    if (h->lift_step == BFM_LIFT_FINISHED) {
        h->lift_data = NULL;
        rest(h);
        return 1;
    }
    if (h->lift_step == BFM_LIFT_ATTACH || h->lift_step == BFM_LIFT_WALK_ATTACH) {
        BfmForestTransform root;
        bfm_forest_actor_root(h->sample.heading, h->sample.feet, &root);
        if (!bfm_forest_lift_grip(data, &h->lift, &root, h->held_position)) return 0;
        if (h->lift_step == BFM_LIFT_WALK_ATTACH) {
            int16_t heading;
            if (!bfm_forest_manual_heading(camera, x, y, &heading)) return 0;
            if (bfm_forest_carry_step(h->surface, &h->actor, heading,
                                      data->walking_speed, h->sample.feet))
                h->sample.heading = heading;
        }
    }
    h->actor.state = h->lift.clip == BFM_LIFT_THROW ? 14 : 13;
    h->sample.elapsed = h->lift.age;
    h->sample.body = data->motion[h->lift.clip].frames[h->lift.age];
    return 1;
}

BfmSteamHeroLumina bfm_steam_hero_lumina(BfmSteamKnightHeroMotion *h,
    const BfmLuminaRotation *rotation, BfmForestStatus *player,
    const BfmForestCamera *camera, int x, int y, uint16_t pressed, uint16_t held,
    BfmSteamFlameRay rays[BFM_ROTATION_RAYS], size_t *count)
{
    if (!h || !rotation ||
        !rotation->held || !player || !camera || !rays || !count ||
        x < -1 || x > 1 || y < -1 || y > 1) return BFM_STEAM_LUMINA_ERROR;
    *count = 0;
    uint8_t *gauge = &player->record[0x47];
    if (h->actor.state != 8) {
        int charging = h->actor.state == 17;
        if (h->jump.active || h->lift_data || (!charging && h->actor.state > 1) ||
            (pressed & (BFM_FOREST_CROSS | BFM_FOREST_SQUARE))) {
            if (h->grip) {
                h->grip = *gauge = 0;
                if (charging) h->actor.state = h->ground.moving ? 1 : 0;
            }
            return BFM_STEAM_LUMINA_PASS;
        }
        int triangle = !!(pressed & BFM_FOREST_TRIANGLE);
        if (!charging && !(held & BFM_FOREST_R1))
            return triangle ? BFM_STEAM_LUMINA_SLASH : BFM_STEAM_LUMINA_PASS;
        int spin = bfm_lumina_charge(&h->grip, gauge, !!(held & BFM_FOREST_R1), triangle);
        if (!spin && triangle) {
            h->grip = *gauge = 0;
            if (charging) h->actor.state = h->ground.moving ? 1 : 0;
            return BFM_STEAM_LUMINA_SLASH;
        }
        if (!spin && !h->grip) {
            if (!charging) return BFM_STEAM_LUMINA_PASS;
            rest(h);
            h->sample.action = 0;
            return BFM_STEAM_LUMINA_FINISHED;
        }
        if (!spin) {
            if (charging) h->actor.state = h->ground.moving ? 1 : 0;
            /* Entering the stance consumes one update. Movement begins on
             * the following update, even if the stick was already held. */
            BfmForestManualResult ground = bfm_forest_stance_step(&h->ground, h->surface,
                &h->actor, camera, charging ? x : 0, charging ? y : 0,
                &h->sample.heading, h->sample.feet);
            if (ground == BFM_FOREST_MANUAL_ERROR) return BFM_STEAM_LUMINA_ERROR;
            if (ground == BFM_FOREST_MANUAL_FALL) {
                h->grip = *gauge = 0;
                if (!bfm_forest_fall_begin(&h->jump, &h->ground, &h->actor))
                    return BFM_STEAM_LUMINA_ERROR;
                return BFM_STEAM_LUMINA_PASS;
            }
            unsigned moving = !!(x || y);
            const BfmForestCastClip *clip = moving ? &h->charge_poses.strafe : &h->charge_poses.stance;
            if (!charging || h->charge_moving != moving) h->charge_key = 0;
            else h->charge_key = (h->charge_key + 1) % clip->count;
            h->charge_moving = moving;
            h->actor.state = h->sample.action = 17;
            h->sample.elapsed = h->charge_key;
            h->sample.body = clip->keys[h->charge_key].pose;
            return BFM_STEAM_LUMINA_STANCE;
        }
        rest(h);
        h->spin_heading = h->sample.heading;
        h->spin_age = 1;
        h->actor.state = 8;
    } else {
        ++h->spin_age;
    }
    const BfmLuminaRotationKey *pose = bfm_lumina_rotation_pose(rotation, h->spin_age,
                                            h->spin_heading, &h->sample.heading);
    if (!pose) {
        rest(h);
        h->sample.action = 0;
        h->sample.heading = h->spin_heading;
        return BFM_STEAM_LUMINA_FINISHED;
    }
    *count = bfm_lumina_rotation_rays(rotation, h->spin_age, h->sample.feet, h->spin_heading, rays);
    if (h->spin_age >= rotation->held + 13) h->sample.heading = h->spin_heading;
    h->sample.action = 8;
    h->sample.elapsed = h->spin_age;
    h->sample.body = pose->body;
    return BFM_STEAM_LUMINA_ROTATE;
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
    if (!h || h->lift_data || h->actor.state == RECOIL) return 0;
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

int bfm_steam_hero_mace_hit(BfmSteamKnightHeroMotion *h, const MusashiDiscMedia *disc,
                            const BfmForestStatus *player, int16_t hit_yaw,
                            uint16_t power, uint16_t *shown)
{
    if (!h || !disc || !player || !shown || !power) return -1;
    *shown = 0;
    if (h->jump.active || h->lift_data || h->actor.field_a8 || h->mace_recovered ||
        h->mace_reaction.active || h->actor.state == RECOIL ||
        h->actor.kind == RECOIL || h->actor.kind == 26 ||
        (h->actor.scene_callback && !(h->actor.scene_callback & UINT32_C(0x02000000))))
        return 0;
    uint16_t damage;
    if (!bfm_forest_hit_damage(disc, player, h->actor.flags, power, &damage) ||
        !bfm_forest_hurt_begin(&h->mace_reaction, h->sample.heading, hit_yaw)) return -1;
    h->mace_damage = *shown = damage;
    /* Charging delegates to the ordinary actor update when interrupted. */
    h->actor.state = RECOIL;
    return 1;
}

static int mace_step(BfmSteamKnightHeroMotion *h)
{
    unsigned key = h->mace_reaction.next_key;
    uint32_t pose;
    BfmForestHurtResult state = bfm_forest_hurt_step(&h->mace_reaction, &h->hurt,
                                                    &h->actor, &h->ground, &pose);
    if (state == BFM_FOREST_HURT_RECOVERED) {
        rest(h);
        h->mace_recovered = 1;
        return 1;
    }
    if (state != BFM_FOREST_HURT_LOCKED) return 0;
    h->sample.body = pose;
    h->sample.motion = STILL;
    h->sample.elapsed = key;
    return 1;
}

int bfm_steam_hero_mace_debit(BfmSteamKnightHeroMotion *h, BfmForestStatus *player)
{
    if (!h || !player) return -1;
    h->mace_blink_history = (h->mace_blink_history << 1) & 7u;
    if (!h->mace_recovered) {
        if (h->actor.field_a8) --h->actor.field_a8;
        h->mace_blink_history |= h->actor.field_a8 & 1u;
        return 0;
    }
    unsigned damage = h->mace_damage;
    if (h->actor.flags & UINT32_C(0x02000000))
        damage = bfm_forest_status_values(player).max_hp;
    bfm_forest_status_debit(player, damage);
    h->mace_damage = 0;
    h->mace_recovered = 0;
    h->actor.field_a8 = 32;
    return 1;
}

int bfm_steam_hero_mace_hidden(const BfmSteamKnightHeroMotion *h)
{
    /* Body submissions retain the protected model state from two updates ago. */
    return h && (h->mace_blink_history & 4u);
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
    h->lift_step = BFM_LIFT_PLAY;
    h->previous_brace = h->braced;
    h->braced = 0;
    if (h->mace_reaction.active) {
        ok = mace_step(h);
    } else if (h->lift_data) {
        ok = carry(h, camera, x, y, pressed);
    } else if (h->actor.state == RECOIL) {
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
