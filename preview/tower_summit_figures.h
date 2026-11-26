/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_SUMMIT_FIGURES_H
#define BFM_TOWER_SUMMIT_FIGURES_H

#include "forest_terrain.h"

typedef struct BfmTowerSummitFigures BfmTowerSummitFigures;
enum { BFM_TOWER_SUMMIT_FIGURES = 3 };

/* Initial summit entrance: the stone head, its pedestal, and the idle switch. */
BfmTowerSummitFigures *bfm_tower_summit_figures_open(const uint8_t *scene, size_t bytes);
void bfm_tower_summit_figures_free(BfmTowerSummitFigures *figures);
size_t bfm_tower_summit_figures_actors(const BfmTowerSummitFigures *figures,
                                     BfmForestActor *out, size_t capacity);

#endif
