/* SPDX-License-Identifier: MIT */
#include "woods_rootrick_wall.h"
#include "woods_rootrick_hero.h"
#include "village_arrival_motion.h"
#include "gameplay_run.h"
#include "forest_lift.h"
#include "forest_pose.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    BFM_RIVAL_CLIMBING, BFM_RIVAL_LOSE_GRIP, BFM_RIVAL_FALLING,
    BFM_RIVAL_GROUNDED, BFM_RIVAL_GETTING_UP, BFM_RIVAL_REBOUNDING,
    BFM_RIVAL_START_CREST, BFM_RIVAL_CREST_POSE, BFM_RIVAL_STEP_ONTO_LEDGE,
    BFM_RIVAL_ON_LEDGE
} BfmRootrickRivalPhase;

typedef struct {
    BfmRootrickRivalPhase phase;
    BfmRootrickRivalStatus status;
    unsigned dust_clock;
    int32_t ledge_step[3];
    int16_t dust_offsets[4][3];
} BfmRootrickRivalMotion;

typedef struct {
    int landed;              /* Arm sound 0xb09 and the scene's impact shake. */
    unsigned dust_sites;     /* Two kind-37 variant-1 actors at each returned site. */
    int16_t dust_xyz[2][3];
} BfmRootrickRivalEffects;

static void rival_start_ascent(BfmRootrickRivalMotion *motion,
                               BfmWoodsRootrickActor *actor, BfmGameplayRun *run)
{
    bfm_woods_rootrick_actor_impulse(actor, actor->climb, 0);
    bfm_woods_rootrick_actor_bind_clip(actor, UINT32_C(0x801f3efc));
    /* The actor samples its initial side even on this vertical wall. */
    (void)bfm_gameplay_random(run);
    actor->timer = 45;
    motion->phase = BFM_RIVAL_CLIMBING;
    motion->status = BFM_ROOTRICK_RACING;
}

static int rival_begin(BfmRootrickRivalMotion *motion, BfmWoodsRootrickActor *actor,
    const uint8_t *scene, size_t bytes, BfmGameplayRun *run)
{
    const size_t header = UINT32_C(0x80190ce4) - UINT32_C(0x80128158);
    const size_t ledge = UINT32_C(0x80195120) - UINT32_C(0x80128158);
    const size_t dust = UINT32_C(0x801955f4) - UINT32_C(0x80128158);
    if (!motion || !actor || !actor->ground || !scene || !run ||
        bytes < dust + 4*8 || u32le(scene+header) != UINT32_C(0x801961e8)) return 0;
    BfmRootrickRivalMotion next = {0};
    for (unsigned axis = 0; axis < 3; ++axis) {
        next.ledge_step[axis] = (int32_t)u32le(scene+ledge+4*axis);
        for (unsigned i = 0; i < 4; ++i)
            next.dust_offsets[i][axis] = s16le(scene+dust+8*i+2*axis);
    }
    rival_start_ascent(&next, actor, run);
    *motion = next;
    return 1;
}

static void rival_climbing_dust(BfmRootrickRivalMotion *motion, BfmGameplayRun *run,
    const BfmForestTransform *root, BfmRootrickRivalEffects *effects)
{
    if (motion->dust_clock && --motion->dust_clock) return;
    motion->dust_clock = 45;
    if (bfm_gameplay_random(run) % 3) return;
    for (unsigned i = 0; i < 2; ++i) {
        unsigned sample = 2*i + (bfm_gameplay_random(run) & 1u);
        int16_t local[3];
        memcpy(local,motion->dust_offsets[sample],sizeof local);
        local[1] -= 20;
        bfm_forest_lift_world(root,local,effects->dust_xyz[i]);
    }
    effects->dust_sites = 2;
}

static int rival_climb(BfmRootrickRivalMotion *motion, BfmWoodsRootrickActor *actor,
    int32_t player_y_q16, const BfmRootrickObstacleField *obstacles,
    BfmGameplayRun *run, BfmRootrickRivalEffects *effects)
{
    int16_t old_xyz[3];
    uint16_t old_fraction[3];
    memcpy(old_xyz,actor->xyz,sizeof old_xyz);
    memcpy(old_fraction,actor->fraction,sizeof old_fraction);
    const int32_t y = (int32_t)actor->xyz[1] * 65536 + actor->fraction[1];
    const int32_t impulse[3] = {0,bfm_rootrick_rival_speed(player_y_q16,y),0};
    unsigned contact;
    bfm_woods_rootrick_actor_impulse(actor,impulse,0);
    if (!bfm_woods_rootrick_actor_move(actor,NULL,&contact)) return 0;
    if (bfm_rootrick_rival_at_crest(actor->xyz[1])) {
        motion->phase = BFM_RIVAL_START_CREST;
        motion->status = BFM_ROOTRICK_PULLING_UP;
        return 1;
    }
    BfmForestTransform previous, proposed;
    bfm_forest_actor_pitch_yaw((unsigned)actor->pitch,actor->yaw,old_xyz,&previous);
    bfm_forest_actor_pitch_yaw((unsigned)actor->pitch,actor->yaw,actor->xyz,&proposed);
    int hit = bfm_rootrick_obstacle_crossing(obstacles,&proposed,&previous,2);
    if (hit < 0 || hit > 2) return 0;
    if (hit) {
        memcpy(actor->xyz,old_xyz,sizeof old_xyz);
        memcpy(actor->fraction,old_fraction,sizeof old_fraction);
        actor->pose_held = 1;
        if (hit == 2) {
            motion->phase = BFM_RIVAL_LOSE_GRIP;
            motion->status = BFM_ROOTRICK_STUMBLING;
        }
        return 1;
    }
    if (actor->pose_held)
        bfm_woods_rootrick_actor_bind_clip(actor,UINT32_C(0x801f3efc));
    rival_climbing_dust(motion,run,&proposed,effects);
    return 1;
}

static int rival_step(BfmRootrickRivalMotion *motion, BfmWoodsRootrickActor *actor,
    int32_t player_y_q16, const BfmRootrickObstacleField *obstacles,
    BfmGameplayRun *run, BfmRootrickRivalEffects *effects)
{
    static const int16_t feet[3] = {0};
    static const int32_t still[3] = {0};
    static const int32_t fall_forward[3] = {0,0,10*65536};
    static const int32_t rebound[3] = {0,0,-7*32768};
    const int32_t gravity = 98304;
    if (!motion || !actor || !actor->ground || !run || !effects) return 0;
    BfmRootrickRivalMotion next = *motion;
    BfmWoodsRootrickActor a = *actor;
    BfmRootrickRivalEffects fx = {0};
    unsigned contact;
    if (!a.pose_held && a.elapsed < UINT_MAX) ++a.elapsed;
    switch (next.phase) {
    case BFM_RIVAL_CLIMBING:
        if (!rival_climb(&next,&a,player_y_q16,obstacles,run,&fx)) return 0;
        break;
    case BFM_RIVAL_LOSE_GRIP:
        bfm_woods_rootrick_actor_impulse(&a,fall_forward,gravity);
        next.phase = BFM_RIVAL_FALLING;
        break;
    case BFM_RIVAL_FALLING:
        a.pitch -= 32;
        if (a.pitch < -1024) a.pitch = -1024;
        if (!bfm_woods_rootrick_actor_move(&a,feet,&contact)) return 0;
        if (contact & 0x2000) {
            a.pitch = -1024;
            a.timer = 30;
            fx.landed = 1;
            next.phase = BFM_RIVAL_GROUNDED;
        }
        break;
    case BFM_RIVAL_GROUNDED:
        if (!a.timer || !--a.timer) next.phase = BFM_RIVAL_GETTING_UP;
        break;
    case BFM_RIVAL_GETTING_UP:
        a.pitch += 128;
        if (a.pitch >= 0) {
            a.pitch = 0;
            bfm_woods_rootrick_actor_impulse(&a,rebound,gravity);
            bfm_woods_rootrick_actor_bind_clip(&a,UINT32_C(0x801f3fa4));
            next.phase = BFM_RIVAL_REBOUNDING;
        }
        break;
    case BFM_RIVAL_REBOUNDING:
        if (!bfm_woods_rootrick_actor_move(&a,a.probe,&contact)) return 0;
        if (contact & 0x8000) rival_start_ascent(&next,&a,run);
        break;
    case BFM_RIVAL_START_CREST:
        bfm_woods_rootrick_actor_bind_clip(&a,UINT32_C(0x801f41dc));
        a.timer = 20;
        next.phase = BFM_RIVAL_CREST_POSE;
        break;
    case BFM_RIVAL_CREST_POSE:
        if (!a.timer || !--a.timer) {
            a.pose_held = 1;
            next.phase = BFM_RIVAL_STEP_ONTO_LEDGE;
        }
        break;
    case BFM_RIVAL_STEP_ONTO_LEDGE:
        bfm_woods_rootrick_actor_bind_clip(&a,UINT32_C(0x801f3ea4));
        bfm_woods_rootrick_actor_impulse(&a,next.ledge_step,0);
        if (!bfm_woods_rootrick_actor_move(&a,NULL,&contact)) return 0;
        bfm_woods_rootrick_actor_impulse(&a,still,gravity);
        a.timer = 16;
        next.phase = BFM_RIVAL_ON_LEDGE;
        next.status = BFM_ROOTRICK_AT_TOP;
        break;
    case BFM_RIVAL_ON_LEDGE:
        break;
    default:
        return 0;
    }
    *actor = a;
    *motion = next;
    *effects = fx;
    return 1;
}

struct BfmRootrickWall {
    const uint8_t *scene;
    size_t scene_bytes;
    BfmRootrickObstacleField props, before_props;
    BfmRootrickHero hero;
    BfmRootrickRivalMotion opponent;
    BfmRootrickFinish finish;
    BfmRootrickWallEffects effects;
    int started, rival_ready, rival_started, retry_closed;
};

BfmRootrickWall *bfm_rootrick_wall_open(const uint8_t *scene, size_t bytes)
{
    BfmRootrickWall *wall = calloc(1, sizeof *wall);
    if (!wall) return NULL;
    if (!bfm_rootrick_obstacles_mount(&wall->props, scene, bytes)) {
        free(wall);
        return NULL;
    }
    wall->scene = scene;
    wall->scene_bytes = bytes;
    wall->before_props = wall->props;
    return wall;
}

void bfm_rootrick_wall_free(BfmRootrickWall *wall)
{
    free(wall);
}

int bfm_rootrick_wall_advance_props(BfmRootrickWall *wall, BfmGameplayRun *run)
{
    if (!wall || !run) return 0;
    /* Musashi probes the model that the preceding frame published. */
    wall->before_props = wall->props;
    bfm_rootrick_obstacles_tick(&wall->props, bfm_gameplay_story_flag(run,158));
    return 1;
}

void bfm_rootrick_wall_challenge(BfmRootrickWall *wall)
{
    if (wall) bfm_rootrick_obstacles_challenge(&wall->props);
}

void bfm_rootrick_wall_hold(BfmRootrickWall *wall, int hold)
{
    if (wall) bfm_rootrick_obstacles_hold(&wall->props, hold);
}

const BfmRootrickObstacleField *bfm_rootrick_wall_props(const BfmRootrickWall *wall)
{
    return wall ? &wall->props : NULL;
}

const BfmRootrickWallEffects *bfm_rootrick_wall_effects(const BfmRootrickWall *wall)
{
    return wall ? &wall->effects : NULL;
}

int bfm_rootrick_wall_attached(const BfmRootrickWall *wall)
{
    return wall && wall->started && wall->hero.attached;
}

int bfm_rootrick_wall_standing(const BfmRootrickWall *wall)
{
    return wall && wall->started && bfm_rootrick_hero_standing(&wall->hero);
}

void bfm_rootrick_wall_retry_closed(BfmRootrickWall *wall)
{
    if (wall && wall->hero.phase == BFM_ROOTRICK_HERO_RETRY) wall->retry_closed = 1;
}

int bfm_rootrick_wall_begin(BfmRootrickWall *wall, BfmVillageMotion *player,
                           const int64_t staged_position[3])
{
    if (!wall || wall->started || !bfm_village_motion_rootrick_begin(player,&wall->hero,staged_position)) return 0;
    wall->started = 1;
    return 1;
}

static int hero_solids(const BfmRootrickObstacleField *props, BfmRootrickHeroObstacle solids[3])
{
    for (unsigned i = 0; i < BFM_ROOTRICK_PLATFORMS; ++i) {
        if (!bfm_rootrick_obstacle_body(props,i,&solids[i].box,&solids[i].transform)) return 0;
        solids[i].body_flags = bfm_rootrick_obstacle_flags(props,i);
        solids[i].present = 1;
    }
    return 1;
}

int bfm_rootrick_wall_step(BfmRootrickWall *wall, BfmVillageMotion *player,
    BfmWoodsRootrickActor *rival, int rival_at_wall, BfmGameplayRun *run,
    BfmFieldInput input, int camera_yaw, unsigned field_tick, BfmRootrickRaceOutcome *outcome)
{
    if (!wall || !wall->started || !player || !rival || !run || !outcome || wall->finish.result) return 0;
    BfmRootrickHeroObstacle before[3], after[3];
    if (!hero_solids(&wall->before_props,before) || !hero_solids(&wall->props,after)) return 0;
    BfmRootrickHeroPhase prior = wall->hero.phase;
    const BfmVillagePlayer *musashi = bfm_village_motion_player(player);
    const int had_grip = wall->hero.attached;
    int16_t previous_xyz[3];
    memcpy(previous_xyz,musashi->root.xyz,sizeof previous_xyz);
    int32_t player_y_q16;
    if (!bfm_village_motion_rootrick_step(player,&wall->hero,before,after,input,camera_yaw,field_tick,
            bfm_gameplay_story_flag(run,31),&player_y_q16)) return 0;
    BfmRootrickRivalEffects rival_effects = {0};
    unsigned attached = !had_grip && wall->hero.attached;
    unsigned hand = had_grip ? musashi->pose.body_contact : 0;
    if (wall->rival_started) {
        int player_y = player_y_q16 / 65536 - (player_y_q16 % 65536 < 0);
        bfm_rootrick_finish_step(&wall->finish,wall->opponent.status,player_y,
                                 wall->hero.grounded,wall->hero.attached);
        if (wall->finish.result == BFM_ROOTRICK_WIN) rival->pose_held = 1;
        if (!wall->finish.result) {
            /* A ridden mover refreshes its model to carry Musashi. The
             * others publish their new matrices after Rootrick's probe. */
            BfmRootrickObstacleField models = wall->before_props;
            if (wall->hero.rider) {
                unsigned i = wall->hero.rider - 1;
                models.platform[i] = wall->props.platform[i];
            }
            if (!rival_step(&wall->opponent,rival,player_y_q16,&models,run,&rival_effects)) return 0;
        }
    } else if (wall->rival_ready) {
        if (!rival_begin(&wall->opponent,rival,wall->scene,wall->scene_bytes,run)) return 0;
        wall->rival_started = 1;
    }
    if (rival_at_wall) wall->rival_ready = 1;
    wall->effects = (BfmRootrickWallEffects){.hand_contact=hand, .attached=attached,
        .hand_sound=attached || hand == 8 ? 0x56du : hand == 9 ? 0x56eu : 0,
        .rival_sound=rival_effects.landed ? 0xb09u : 0,
        .rival_dust_sites=rival_effects.dust_sites};
    if (attached || hand) {
        /* Alternating strikes emit before the stroke lifts Musashi. */
        memcpy(wall->effects.hand_xyz,attached ? musashi->root.xyz : previous_xyz,
               sizeof wall->effects.hand_xyz);
        wall->effects.hand_xyz[1] -= 20;
    }
    memcpy(wall->effects.rival_dust_xyz,rival_effects.dust_xyz,sizeof rival_effects.dust_xyz);
    if (!wall->finish.result && wall->hero.phase == BFM_ROOTRICK_HERO_RETRY) {
        if (prior != BFM_ROOTRICK_HERO_RETRY) wall->effects.retry_message = UINT32_C(0x80196530);
        if (wall->hero.grounded && wall->retry_closed) {
            wall->retry_closed = 0;
            bfm_rootrick_hero_begin(&wall->hero);
        }
    }
    *outcome = wall->finish.result;
    return 1;
}
