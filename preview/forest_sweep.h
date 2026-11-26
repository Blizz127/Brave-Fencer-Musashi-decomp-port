/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_SWEEP_H
#define BFM_FOREST_SWEEP_H

#include "forest_field.h"

typedef struct {
    int16_t point[2][3]; /* Fusion tip and hilt in world space. */
} BfmForestSweepEdge;

typedef struct {
    BfmForestSweepEdge edges[BFM_FOREST_SWEEP_CARDS + 1];
    unsigned count, delay, duration, divisor, energy;
} BfmForestSweep;

/* Begin the first grounded Fusion swing on its logical issue update. Step on
 * every subsequent gameplay update, including recovery. A zero logical pose
 * ends sampling and lets the luminous history fade.
 * The model root is the one in use before selecting the next display pose. */
int bfm_forest_sweep_begin(BfmForestSweep *sweep, const BfmForestField *field);
int bfm_forest_sweep_step(BfmForestSweep *sweep, const MusashiDiscMedia *disc,
                          uint32_t logical_pose, const BfmForestTransform *model);
unsigned bfm_forest_sweep_project(const BfmForestSweep *sweep, const BfmForestCamera *camera,
                                  BfmForestDustCard out[BFM_FOREST_SWEEP_CARDS]);
#endif
