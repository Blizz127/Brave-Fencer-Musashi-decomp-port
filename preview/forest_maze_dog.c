/* SPDX-License-Identifier: MIT */
#include "forest_maze_dog.h"
#include "field_waypoint.h"
#include "forest_pose.h"
#include "../pc_port/forest_player_bytes.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

enum GuidePhase { ABSENT, APPROACH, BRAKING, SEARCHING, LEADING, WAITING };
enum { UNIT = 65536, GUIDE_VARIABLE = 204 };

struct BfmMazeDog {
    BfmMazeDogView view;
    enum GuidePhase phase;
    const BfmVillageGround *ground;
    const uint8_t *targets;
    int16_t center[3];
    int32_t stride, sniff_stride, braking_speed;
    int fraction[2];
    unsigned direction, turn_timer, search_timer, sound_timer, wall_timer;
    unsigned entry_sound;
    int final_target, correct_trail, bark;
};

static int earned_stage(unsigned stage)
{
    return stage >= 2 && stage <= 12 && !(stage & 1u);
}

BfmMazeDog *bfm_maze_dog_open(const uint8_t *scene, size_t bytes, const BfmVillageGround *ground,
                              const int16_t center[3], const BfmBedroomRoot *player,
                              const BfmSaveRecord *save)
{
    const size_t motion = UINT32_C(0x8018c03c) - UINT32_C(0x80128158);
    if (!scene || !ground || !center || !player || !save ||
        bytes < motion + 56) return NULL;
    BfmMazeDog *dog = calloc(1, sizeof *dog);
    if (!dog) return NULL;
    unsigned stage = save->variables[GUIDE_VARIABLE];
    if (!earned_stage(stage)) {
        if (u32le(save->progress + 8) == 50) dog->entry_sound = 3149;
        return dog;
    }
    const uint8_t *vectors = scene + motion;
    if (u32le(vectors) || u32le(vectors+4) || u32le(vectors+12) || u32le(vectors+16) ||
        u16le(vectors+8) || u16le(vectors+20)) goto invalid;
    int stride = s16le(vectors+10), sniff_stride = s16le(vectors+22);
    if (stride >= 0 || stride < -128 || sniff_stride >= 0 || sniff_stride < stride) goto invalid;
    dog->stride = stride*UNIT;
    dog->sniff_stride = sniff_stride*UNIT;
    dog->targets = vectors+24;
    dog->ground = ground;
    memcpy(dog->center, center, sizeof dog->center);
    BfmBedroomRoot anchor = {0};
    memcpy(anchor.xyz, center, sizeof anchor.xyz);
    if (anchor.xyz[1] > INT16_MAX-16) goto invalid;
    anchor.xyz[1] += 16;
    if (!bfm_bedroom_travel_air_shadow(scene, bytes, UINT32_C(0x8018c890), &anchor) ||
        !anchor.shadow_visible) goto invalid;
    dog->view.xyz[0] = player->xyz[0];
    dog->view.xyz[1] = anchor.shadow_xyz[1];
    dog->view.xyz[2] = player->xyz[2];
    dog->phase = APPROACH;
    return dog;
invalid:
    free(dog);
    return NULL;
}

void bfm_maze_dog_free(BfmMazeDog *dog)
{
    free(dog);
}

const BfmMazeDogView *bfm_maze_dog_view(const BfmMazeDog *dog)
{
    return dog && dog->phase != ABSENT ? &dog->view : NULL;
}

static void start_clip(BfmMazeDog *dog, BfmMazeDogClip clip)
{
    dog->view.clip = clip;
    dog->view.tick = 0;
}

static void turn_toward(BfmMazeDog *dog, int x, int z, int divisor)
{
    int heading = bfm_waypoint_heading(dog->view.xyz[0]-x, dog->view.xyz[2]-z);
    int delta = heading - (int)dog->view.yaw;
    if (delta > 2048) delta -= 4096;
    if (delta < -2048) delta += 4096;
    int step = delta/divisor;
    if (!step && delta) step = delta > 0 ? 1 : -1;
    dog->view.yaw = (dog->view.yaw + (unsigned)step) & 4095u;
}

/* Retain fractional X/Z across fields, including the slow search and braking. */
static int advance(BfmMazeDog *dog, int32_t speed)
{
    BfmForestTransform heading;
    bfm_forest_actor_root(dog->view.yaw, dog->view.xyz, &heading);
    int16_t to[3];
    memcpy(to, dog->view.xyz, sizeof to);
    int intended[2], remainder[2];
    for (unsigned i = 0; i < 2; ++i) {
        unsigned axis = i*2;
        int64_t displacement = (int64_t)speed*heading.rotation[axis][2];
        int fixed = (int)(displacement/4096 - (displacement%4096 < 0)) + dog->fraction[i];
        int whole = fixed/UNIT - (fixed%UNIT < 0);
        intended[i] = dog->view.xyz[axis]+whole;
        if (intended[i] < INT16_MIN || intended[i] > INT16_MAX) return 1;
        remainder[i] = fixed-whole*UNIT;
        to[axis] = (int16_t)intended[i];
    }
    BfmForestContact floor;
    if (bfm_village_ground_step(dog->ground, dog->view.xyz, to, &floor) != BFM_VILLAGE_GROUND_FOOTING)
        return 1;
    int wall = intended[0] != to[0] || intended[1] != to[2];
    for (unsigned i = 0; i < 2; ++i) dog->fraction[i] = intended[i] == to[2*i] ? remainder[i] : 0;
    memcpy(dog->view.xyz, to, sizeof to);
    return wall;
}

static void approach(BfmMazeDog *dog)
{
    int x = dog->view.xyz[0]-dog->center[0], z = dog->view.xyz[2]-dog->center[2];
    if (x < -400) dog->view.yaw = 3072;
    else if (x > 400) dog->view.yaw = 1024;
    else if (z < -400) dog->view.yaw = 2048;
    else if (z > 400) dog->view.yaw = 0;
    else {
        dog->phase = BRAKING;
        dog->braking_speed = -12*UNIT;
        start_clip(dog, BFM_MAZE_DOG_BRAKE);
    }
    advance(dog, dog->stride);
}

static void start_search(BfmMazeDog *dog, unsigned stage, BfmGameplayRun *run)
{
    if (!earned_stage(stage)) {
        dog->phase = ABSENT;
        return;
    }
    dog->phase = SEARCHING;
    start_clip(dog, BFM_MAZE_DOG_SEARCH);
    dog->turn_timer = 1 + (bfm_gameplay_random(run) & 3u);
    dog->sound_timer = 1;
    dog->search_timer = 180;
    dog->correct_trail = !!(bfm_gameplay_random(run) & 3u);
    if (!dog->correct_trail) {
        dog->direction = bfm_gameplay_random(run) & 3u;
        return;
    }
    dog->direction = stage == 2 || stage == 4 ? 2u : stage == 8 ? 3u : 0u;
    dog->search_timer += 15u * (stage-2u);
}

static void target(const BfmMazeDog *dog, int *x, int *z)
{
    const uint8_t *pair = dog->targets + 16*dog->final_target + 2*dog->direction;
    *x = dog->center[0] + s16le(pair);
    *z = dog->center[2] + s16le(pair+8);
}

static unsigned search(BfmMazeDog *dog, BfmGameplayRun *run)
{
    unsigned sound = 0;
    advance(dog, dog->sniff_stride);
    if (!--dog->sound_timer) {
        sound = 1319;
        dog->sound_timer = 15;
    }
    if (!--dog->turn_timer) {
        dog->turn_timer = 1 + (bfm_gameplay_random(run) & 3u);
        dog->view.yaw = (dog->view.yaw + (bfm_gameplay_random(run)&511u) - 256u) & 4095u;
        int x, z;
        target(dog, &x, &z);
        turn_toward(dog, x, z, 8);
    }
    if (!--dog->search_timer) {
        dog->phase = LEADING;
        dog->final_target = 0;
        start_clip(dog, BFM_MAZE_DOG_RUN);
    }
    return sound;
}

static void lead(BfmMazeDog *dog, BfmSaveRecord *save, BfmGameplayRun *run)
{
    if (advance(dog, dog->stride)) {
        dog->wall_timer = (dog->wall_timer+1u)&63u;
        dog->view.yaw = (dog->view.yaw + (dog->wall_timer < 31 ? 512u : 3584u)) & 4095u;
    }
    int x, z;
    target(dog, &x, &z);
    turn_toward(dog, x, z, 4);
    int64_t dx = dog->view.xyz[0]-x, dz = dog->view.xyz[2]-z;
    if (dx*dx + dz*dz >= 32*32) return;
    if (!dog->final_target) {
        dog->final_target = 1;
    } else if (!dog->correct_trail) {
        dog->phase = APPROACH;
        start_clip(dog, BFM_MAZE_DOG_RUN);
    } else {
        dog->phase = WAITING;
        dog->sound_timer = 15;
        dog->bark = 1;
        start_clip(dog, BFM_MAZE_DOG_REST);
        unsigned stage = save->variables[GUIDE_VARIABLE];
        if (earned_stage(stage)) {
            save->variables[GUIDE_VARIABLE] = (uint8_t)(stage+1);
            run->banks.variables[GUIDE_VARIABLE] = (uint8_t)(stage+1);
        }
    }
}

unsigned bfm_maze_dog_step(BfmMazeDog *dog, const BfmBedroomRoot *player,
                           BfmSaveRecord *save, BfmGameplayRun *run)
{
    if (!dog || !player || !save || !run) return 0;
    if (dog->phase == ABSENT) {
        unsigned sound = dog->entry_sound;
        dog->entry_sound = 0;
        return sound;
    }
    ++dog->view.tick;
    switch (dog->phase) {
    case APPROACH:
        approach(dog);
        break;
    case BRAKING:
        dog->braking_speed += UNIT*5/4;
        advance(dog, dog->braking_speed);
        if (dog->braking_speed >= -2*UNIT) {
            dog->final_target = 0;
            start_search(dog, save->variables[GUIDE_VARIABLE], run);
        }
        break;
    case SEARCHING:
        return search(dog, run);
    case LEADING:
        lead(dog, save, run);
        break;
    case WAITING:
        turn_toward(dog, player->xyz[0], player->xyz[2], 4);
        if (!--dog->sound_timer) {
            dog->bark = !dog->bark;
            start_clip(dog, dog->bark ? BFM_MAZE_DOG_BARK : BFM_MAZE_DOG_REST);
            dog->sound_timer = dog->bark ? 5 : 15;
            if (dog->bark) return 1317;
        }
        break;
    case ABSENT: break;
    }
    return 0;
}
