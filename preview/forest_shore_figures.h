/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_SHORE_FIGURES_H
#define BFM_FOREST_SHORE_FIGURES_H
#include "forest_field.h"

typedef struct BfmForestShoreFigures BfmForestShoreFigures;

/* The two near-shore placements each have a moving body and a rigid base.
 * The mounted field must outlive the figures. */
BfmForestShoreFigures *bfm_forest_shore_figures_open(const BfmForestField *field);
void bfm_forest_shore_figures_free(BfmForestShoreFigures *figures);

/* Called after the player's position has been committed for this field update. */
void bfm_forest_shore_figures_tick(BfmForestShoreFigures *figures,
                                   const int16_t position[3]);

/* Return complete parent/base pairs for the room's shared depth pass.
 * If the destination is too small, leave it untouched. */
size_t bfm_forest_shore_figures_actors(const BfmForestShoreFigures *figures,
                                      BfmForestActor *out, size_t capacity);
#endif
