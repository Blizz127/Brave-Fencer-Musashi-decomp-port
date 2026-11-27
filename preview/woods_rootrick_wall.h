/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_ROOTRICK_WALL_H
#define BFM_WOODS_ROOTRICK_WALL_H

#include "woods_rootrick_actor.h"
#include "woods_rootrick_obstacles.h"
#include "woods_rootrick_race.h"

typedef struct BfmRootrickWall BfmRootrickWall;
typedef struct BfmVillageMotion BfmVillageMotion;
typedef struct BfmGameplayRun BfmGameplayRun;

/* Borrow SC02/11 for the room's lifetime. Its three moving ledges start when
 * the room loads, including fields before the challenge and after the race. */
BfmRootrickWall *bfm_rootrick_wall_open(const uint8_t *scene, size_t bytes);
void bfm_rootrick_wall_free(BfmRootrickWall *wall);
int bfm_rootrick_wall_advance_props(BfmRootrickWall *wall, BfmGameplayRun *run);
void bfm_rootrick_wall_challenge(BfmRootrickWall *wall);
void bfm_rootrick_wall_hold(BfmRootrickWall *wall, int hold);
const BfmRootrickObstacleField *bfm_rootrick_wall_props(const BfmRootrickWall *wall);

typedef struct {
    unsigned hand_contact; /* 8: Fusion, 9: Lumina. One field per sword strike. */
    unsigned hand_sound, rival_sound;
    uint32_t retry_message; /* Mounted bottom-landing page, once on entry. */
    unsigned rival_dust_sites;
    int16_t rival_dust_xyz[2][3];
    unsigned attached; /* First sword insertion, before alternating strokes. */
    int16_t hand_xyz[3]; /* World position of the two emitted wall chips. */
} BfmRootrickWallEffects;
const BfmRootrickWallEffects *bfm_rootrick_wall_effects(const BfmRootrickWall *wall);
int bfm_rootrick_wall_attached(const BfmRootrickWall *wall);
int bfm_rootrick_wall_standing(const BfmRootrickWall *wall);
/* Acknowledge the actual bottom-landing window on its ordinary dismissal. */
void bfm_rootrick_wall_retry_closed(BfmRootrickWall *wall);

/* Start on the rival actor's START_WALL signal. The local player begins at
 * the staged field root. Its own approach controls when grip input opens. */
int bfm_rootrick_wall_begin(BfmRootrickWall *wall, BfmVillageMotion *player,
                           const int64_t staged_position[3]);
/* One field following begin. rival_at_wall is the pre-race actor's one-shot
 * AT_WALL signal. On the subsequent field this takes over that same actor.
 * The village motion publishes the actual player pose for the usual renderer. */
int bfm_rootrick_wall_step(BfmRootrickWall *wall, BfmVillageMotion *player,
    BfmWoodsRootrickActor *rival, int rival_at_wall, BfmGameplayRun *run,
    BfmFieldInput input, int camera_yaw, unsigned field_tick, BfmRootrickRaceOutcome *outcome);

#endif
