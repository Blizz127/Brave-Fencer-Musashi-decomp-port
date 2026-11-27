/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_DRIFT_H
#define BFM_FOREST_DRIFT_H

#include "forest_dust.h"
#include "gameplay_run.h"

struct BfmForestField;

/* Ten entrance births plus at most 37 overlapping 15-update births. */
enum { BFM_FOREST_MAZE_DRIFT_CARDS = 47 };

typedef struct {
    int32_t x;
    int16_t y;
    uint16_t scale;
} BfmForestDriftSprite;

typedef struct {
    const uint8_t *heights, *sprite;
    unsigned height_count, next_height, timer, current, submitted, interval, slots;
    BfmForestDriftSprite frames[3][BFM_FOREST_MAZE_DRIFT_CARDS];
} BfmForestDrift;

/* Inputs are mounted spans of 16 signed heights and a 12-byte sprite. */
int bfm_forest_drift_init(BfmForestDrift *drift, const uint8_t heights[32],
                          const uint8_t sprite[12], unsigned variant);
int bfm_forest_drift_load(BfmForestDrift *drift, const struct BfmForestField *field,
                          uint32_t placements);
void bfm_forest_drift_start(BfmForestDrift *drift, BfmGameplayRun *run);
void bfm_forest_drift_tick(BfmForestDrift *drift, BfmGameplayRun *run);
unsigned bfm_forest_drift_count(const BfmForestDrift *drift);
unsigned bfm_forest_drift_project_cards(const BfmForestDrift *drift,
                                        BfmForestDustCard *out, unsigned capacity);
unsigned bfm_forest_drift_project(const BfmForestDrift *drift,
                                  BfmForestDustCard out[BFM_FOREST_DRIFT_CARDS]);
#endif
