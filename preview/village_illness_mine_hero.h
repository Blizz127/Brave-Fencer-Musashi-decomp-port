/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_ILLNESS_MINE_HERO_H
#define BFM_VILLAGE_ILLNESS_MINE_HERO_H

#include "village_arrival_motion.h"
#include "village_camera.h"
#include "village_illness_mine_gate.h"

typedef struct BfmVillageMineHero BfmVillageMineHero;

/* Borrow the lower street and its live player when the gate accepts Square. */
BfmVillageMineHero *bfm_village_mine_hero_open(const uint8_t *scene, size_t bytes,
                                              BfmVillageMotion *motion);
void bfm_village_mine_hero_free(BfmVillageMineHero *hero);

/* On every locked player visit, replace the ordinary input/animation step. */
int bfm_village_mine_hero_step(BfmVillageMineHero *hero, const BfmVillageCamera *view);
/* Follow every gate visit, including the opening and empty frames. The
 * director installs at most one older order before appending today's rows. */
int bfm_village_mine_hero_orders(BfmVillageMineHero *hero, const BfmMineGateFrame *frame,
                                const BfmVillageCamera *view);
int bfm_village_mine_hero_ready(const BfmVillageMineHero *hero);
/* Return control at the reached foot after the gate and camera have released. */
int bfm_village_mine_hero_release(BfmVillageMineHero *hero);

#endif
