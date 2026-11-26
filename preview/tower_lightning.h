/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_LIGHTNING_H
#define BFM_TOWER_LIGHTNING_H

#include "forest_dust.h"
#include "gameplay_run.h"

enum { BFM_TOWER_LIGHTNING_CARDS = 128 };
typedef struct BfmTowerCircuit BfmTowerCircuit;

/* Read the four pairs of conductor endpoints from Spiral Tower's resident
 * room member. The caller may release the room bytes after loading. */
BfmTowerCircuit *bfm_tower_lightning_load(const uint8_t *room, size_t bytes);
void bfm_tower_lightning_free(BfmTowerCircuit *circuit);

/* Begin ticking on the simulation update after the controller appears. Set
 * bit i when the live pedestal with ordinal i has less than three HP.
 * A broken link stays dark until the room is reloaded. */
void bfm_tower_lightning_tick(BfmTowerCircuit *circuit, BfmGameplayRun *gameplay,
                              unsigned broken_pylons);

/* Project a cached update (0=current, 2=displayed) of world ribbons with the active field view. Drawing has
 * no effect on gameplay RNG; multiple views of one tick use the same shape. */
size_t bfm_tower_lightning_cards(const BfmTowerCircuit *circuit, unsigned delay,
                                 const BfmForestCamera *camera,
                                 BfmForestDustCard *out, size_t capacity);
#endif
