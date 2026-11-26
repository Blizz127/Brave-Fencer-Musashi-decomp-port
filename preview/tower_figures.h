/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_FIGURES_H
#define BFM_TOWER_FIGURES_H

#include "forest_dust.h"

enum { BFM_TOWER_FIGURE_ACTORS = 8, BFM_TOWER_FIGURE_CARDS = 28 };
typedef struct BfmTowerFigures BfmTowerFigures;

/* The unpacked SC01 room contains the exterior body, its three children,
 * and the same mounted seven-part figure placed at four corners. */
BfmTowerFigures *bfm_tower_figures_open(const uint8_t *scene, size_t bytes);
void bfm_tower_figures_free(BfmTowerFigures *figures);
/* Age zero is the field update that installs these actors. Returned actor
 * objects borrow the figures until the next call or destruction. */
size_t bfm_tower_figures_actors(BfmTowerFigures *figures, unsigned age,
                                BfmForestActor *out, size_t capacity);
/* The textured core advances with its actor. The halo follows the field clock. */
size_t bfm_tower_figures_cards(const BfmTowerFigures *figures, unsigned age,
                               unsigned lit_nodes, unsigned field_parity,
                               const BfmForestCamera *camera, BfmForestDustCard *out, size_t capacity);
#endif
