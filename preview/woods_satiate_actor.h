/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_SATIATE_ACTOR_H
#define BFM_WOODS_SATIATE_ACTOR_H

#include "bedroom_figure.h"
#include "gameplay_run.h"

typedef struct BfmWoodsSatiateActor BfmWoodsSatiateActor;

/* The room member stays mounted for this actor's lifetime. */
BfmWoodsSatiateActor *bfm_woods_satiate_actor_load(const uint8_t *scene, size_t bytes);
void bfm_woods_satiate_actor_free(BfmWoodsSatiateActor *actor);
int bfm_woods_satiate_actor_step(BfmWoodsSatiateActor *actor, const BfmBedroomView *view,
                                 const BfmBedroomRoot *player, BfmGameplayRun *run);
int bfm_woods_satiate_actor_add(const BfmWoodsSatiateActor *actor, BfmBedroomFigure *figure,
                                const BfmBedroomView *view);
int bfm_woods_satiate_actor_face(const BfmWoodsSatiateActor *actor, uint16_t tiles[512][1024]);
/* Bite or water entry cue from the last actor update; zero means silence. */
unsigned bfm_woods_satiate_actor_sound(const BfmWoodsSatiateActor *actor);
/* A connected enemy strike on this update. The player owns defense and damage. */
unsigned bfm_woods_satiate_actor_strike(const BfmWoodsSatiateActor *actor);
/* Attacker's model heading at the connected strike, in 4096 units per turn.
 * Consult strike first: zero is also a valid heading. */
unsigned bfm_woods_satiate_actor_strike_yaw(const BfmWoodsSatiateActor *actor);

int bfm_woods_satiate_actor_contact(const BfmWoodsSatiateActor *actor, const int16_t start[3],
                                    const int16_t end[3], int16_t contact[3], int16_t center[3]);
int bfm_woods_satiate_actor_target(const BfmWoodsSatiateActor *actor, int16_t center[3]);
int bfm_woods_satiate_actor_root(const BfmWoodsSatiateActor *actor, int16_t root[3]);
int bfm_woods_satiate_actor_charged_contact(const BfmWoodsSatiateActor *actor, const int16_t from[3],
                                           const int16_t to[3], int16_t hit[3], int16_t normal[3]);
int bfm_woods_satiate_actor_charged_target(const BfmWoodsSatiateActor *actor, int16_t root[3]);
void bfm_woods_satiate_actor_charged_recoil(BfmWoodsSatiateActor *actor, unsigned heading);
int bfm_woods_satiate_actor_latch(BfmWoodsSatiateActor *actor);
void bfm_woods_satiate_actor_meter(BfmWoodsSatiateActor *actor, unsigned meter);
void bfm_woods_satiate_actor_release(BfmWoodsSatiateActor *actor);
int bfm_woods_satiate_actor_absorb(BfmWoodsSatiateActor *actor);
#endif
