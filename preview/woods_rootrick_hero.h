/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_ROOTRICK_HERO_H
#define BFM_WOODS_ROOTRICK_HERO_H

#include "bedroom_travel.h"
#include "forest_guard_contact.h"
#include "village_ground.h"
#include "woods_rootrick_race.h"

typedef struct BfmRootrickHeroObstacle {
    BfmForestGuardBox box;
    BfmForestTransform transform;
    int present;
    uint16_t body_flags;
} BfmRootrickHeroObstacle;

typedef enum {
    BFM_ROOTRICK_HERO_APPROACH, BFM_ROOTRICK_HERO_RUNUP, BFM_ROOTRICK_HERO_JUMP,
    BFM_ROOTRICK_HERO_GRIP, BFM_ROOTRICK_HERO_VAULT, BFM_ROOTRICK_HERO_LANDING,
    BFM_ROOTRICK_HERO_RETRY
} BfmRootrickHeroPhase;

typedef struct BfmRootrickHero {
    BfmRootrickHeroPhase phase;
    int grounded, attached;
    int32_t vertical_speed, forward_speed;
    BfmRootrickGrip grip;
    unsigned rider, kick_ticks, cooldown, walking;
    int16_t previous_z[3], previous_foot[3];
    unsigned previous_mask;
} BfmRootrickHero;

/* Called on the challenge's start-wall signal or after the base retry message.
 * The player's live position, fractions, facing and motion remain with the field. */
void bfm_rootrick_hero_begin(BfmRootrickHero *hero);

/* Ordinary standing action, including the grounded wait at the finish. */
int bfm_rootrick_hero_standing(const BfmRootrickHero *hero);

/* One gameplay field. Supply live solids in the room's three placement slots,
 * before their world tick, the field clock, and the learned-climb/equipment check.
 * position is the village actor's Q16 world position. root and pose belong to
 * that same actor. The room monitors grounded/attached and owns the rival,
 * dialogue, finish test and release of script control. */
int bfm_rootrick_hero_step(BfmRootrickHero *hero, BfmBedroomMotion *motion,
    const BfmVillageGround *ground, const BfmRootrickHeroObstacle obstacles[3],
    BfmFieldInput input, int camera_yaw, unsigned field_tick, int can_climb,
    int64_t position[3], BfmBedroomRoot *root, BfmBedroomPose *pose);

/* Apply a platform's motion after the world ticks it. This changes the physical
 * position for the next field. The submitted player pose and camera use the
 * position from the preceding hero step. Repeated calls do not apply it twice. */
int bfm_rootrick_hero_carry_after(BfmRootrickHero *hero, const BfmVillageGround *ground,
    const BfmRootrickHeroObstacle after[3], int64_t position[3]);

#endif
