/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_GLOW_FIGURES_H
#define BFM_FOREST_GLOW_FIGURES_H

#include "forest_glows.h"

typedef struct BfmForestGlowFigures BfmForestGlowFigures;

/* The four wing parts and their animation keys come from the same forest room
 * that owns the lights. Mesh and decoded poses survive closing the disc. */
BfmForestGlowFigures *bfm_forest_glow_figures_open(const BfmForestField *field);
void bfm_forest_glow_figures_free(BfmForestGlowFigures *figures);
unsigned bfm_forest_glow_figures_collect(const BfmForestGlowFigures *figures,
                                         const BfmForestGlows *lights,
                                         BfmForestActor out[BFM_FOREST_GLOWS]);
#endif
