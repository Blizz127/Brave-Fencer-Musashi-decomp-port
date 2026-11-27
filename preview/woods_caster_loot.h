/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_CASTER_LOOT_H
#define BFM_WOODS_CASTER_LOOT_H

#include "bedroom_figure.h"
#include "gameplay_run.h"

typedef struct BfmWoodsCasterLoot BfmWoodsCasterLoot;

/* The wooded room's scene stays mounted until this collection is freed. */
BfmWoodsCasterLoot *bfm_woods_caster_loot_open(const uint8_t *scene, size_t bytes);
void bfm_woods_caster_loot_free(BfmWoodsCasterLoot *loot);

/* Call once on ordinary caster defeat, after existing pickups' update.
 * The call includes the new child's first toss update. Collect it on a
 * later player update. A successful absorption does not issue this drop. */
int bfm_woods_caster_loot_drop(BfmWoodsCasterLoot *loot, BfmGameplayRun *run,
                               const int16_t caster_root[3]);

/* Advance once per active field update. The first reachable pickup credits
 * run->player and returns its sound through sound (zero on a quiet update).
 * Pass allow_pickup only during the normal player contact scan. */
int bfm_woods_caster_loot_step(BfmWoodsCasterLoot *loot, BfmGameplayRun *run,
                               const int16_t hero[3], int allow_pickup, unsigned *sound);
int bfm_woods_caster_loot_add(const BfmWoodsCasterLoot *loot, BfmBedroomFigure *figure,
                              const BfmBedroomView *view, unsigned field_tick);
#endif
