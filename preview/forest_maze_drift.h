/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_MAZE_DRIFT_H
#define BFM_FOREST_MAZE_DRIFT_H

#include "forest_drift.h"
#include <stddef.h>

/* Entrance 0 has the dense screen-space emitter. The later entrance 33
 * selects another placement list. Keep its emitter inactive. */
int bfm_forest_maze_drift_load(BfmForestDrift *drift, const uint8_t *scene,
                               size_t bytes, unsigned entrance);
unsigned bfm_forest_maze_drift_project(const BfmForestDrift *drift,
                                       BfmForestDustCard out[BFM_FOREST_MAZE_DRIFT_CARDS]);
#endif
