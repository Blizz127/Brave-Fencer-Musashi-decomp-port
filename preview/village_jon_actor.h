/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_JON_ACTOR_H
#define BFM_VILLAGE_JON_ACTOR_H
#include "bedroom_figure.h"

typedef struct BfmVillageJonActor BfmVillageJonActor;

/* The caller retains the mounted lower-village scene for the actor's lifetime. */
BfmVillageJonActor *bfm_village_jon_actor_open(const uint8_t *scene, size_t bytes,
                                             uint32_t room);
void bfm_village_jon_actor_free(BfmVillageJonActor *actor);
const int16_t *bfm_village_jon_actor_position(const BfmVillageJonActor *actor);
/* Eligibility for the stocks conversation. The field dispatches an ordinary
 * Square edge after its player, carry and active-dialogue checks. */
int bfm_village_jon_can_talk(const BfmVillageJonActor *actor, unsigned story,
                             const int16_t player[3]);
/* The ordinary exit from the first food-and-water request restarts the clip. */
void bfm_village_jon_actor_reset(BfmVillageJonActor *actor);
void bfm_village_jon_actor_step(BfmVillageJonActor *actor);
int bfm_village_jon_actor_add(const BfmVillageJonActor *actor, unsigned story,
                              BfmBedroomFigure *figure, const BfmBedroomView *view);
#endif
