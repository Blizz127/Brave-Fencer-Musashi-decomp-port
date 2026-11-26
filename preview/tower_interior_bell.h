/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_INTERIOR_BELL_H
#define BFM_TOWER_INTERIOR_BELL_H

#include "forest_terrain.h"

typedef struct BfmTowerInteriorBell BfmTowerInteriorBell;

/* Open the pre-hit bell from the mounted US interior scene. The mesh and
 * placement remain valid after the caller releases the scene bytes. */
BfmTowerInteriorBell *bfm_tower_interior_bell_open(const uint8_t *scene, size_t size);
void bfm_tower_interior_bell_free(BfmTowerInteriorBell *bell);

/* Age zero is the bell's construction update. The returned actor borrows its
 * root and pose until the next sample or release. */
int bfm_tower_interior_bell_actor(BfmTowerInteriorBell *bell, unsigned elapsed_updates,
                                  BfmForestActor *out);
#endif
