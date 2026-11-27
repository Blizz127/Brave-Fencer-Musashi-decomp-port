/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_SHRINK_RIBBON_H
#define BFM_WOODS_SHRINK_RIBBON_H

#include "forest_dust.h"
#include "woods_shrink_shot.h"

enum { BFM_WOODS_SHRINK_RIBBON_CARDS = 4 * 11 };

/* All four strands travel from the frozen cast root. Reconstructing their
 * shared trail from the shot's field age keeps repeated draws identical. */
size_t bfm_woods_shrink_ribbon_project(const BfmWoodsShrinkShot *shot,
    const BfmForestCamera *camera, BfmForestDustCard *out, size_t capacity);

#endif
