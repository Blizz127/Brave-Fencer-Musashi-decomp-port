/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_BARRIER_ART_H
#define BFM_FOREST_BARRIER_ART_H

#include "forest_field.h"

typedef struct BfmForestBarrierArt BfmForestBarrierArt;

/* All five movable boulders share the same rigid room mesh. */
BfmForestBarrierArt *bfm_forest_barrier_art_open(const BfmForestField *field);
void bfm_forest_barrier_art_free(BfmForestBarrierArt *art);

/* The actor borrows its root from the placement and its mesh and pose from art.
 * Keep both owners alive until the room has drawn this actor. */
int bfm_forest_barrier_art_actor(const BfmForestBarrierArt *art,
                                const BfmForestTransform *root, BfmForestActor *out);

#endif
