/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_SUMMIT_FIGURES_H
#define BFM_TOWER_SUMMIT_FIGURES_H

#include "forest_terrain.h"

typedef struct BfmTowerSummitFigures BfmTowerSummitFigures;
enum { BFM_TOWER_SUMMIT_FIGURES = 5 };

/* The stone head, pedestal, plate, upright sword and enclosing crystal.
 * The two sword models share the placement of the pickup actor. */
BfmTowerSummitFigures *bfm_tower_summit_figures_open(const uint8_t *scene, size_t bytes);
void bfm_tower_summit_figures_free(BfmTowerSummitFigures *figures);
int bfm_tower_summit_figures_targets(const BfmTowerSummitFigures *figures,
                                    int16_t plate[3], int16_t sword[3]);
/* One call per simulation update. Settle is the actor's 0..16 scale
 * counter; shielded is its shield state after plate debounce. */
void bfm_tower_summit_figures_present_sword(BfmTowerSummitFigures *figures, unsigned tick,
                                           unsigned settle, int shielded, int sword_present);
void bfm_tower_summit_figures_update_sword(BfmTowerSummitFigures *figures, unsigned tick,
                                          int plate_occupied, int sword_present);
size_t bfm_tower_summit_figures_actors(const BfmTowerSummitFigures *figures,
                                     BfmForestActor *out, size_t capacity);

#endif
