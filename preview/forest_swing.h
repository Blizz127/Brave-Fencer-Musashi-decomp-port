/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_SWING_H
#define BFM_FOREST_SWING_H

#include "musashi_disc_media.h"

enum { BFM_FOREST_SWING_KEYS = 32 };

typedef struct {
    uint32_t pose;
    unsigned marker, window;
    int8_t tip[3]; /* Model-local Fusion tip from the head of this pose record. */
} BfmForestSwingFrame;

typedef struct {
    BfmForestSwingFrame frames[BFM_FOREST_SWING_KEYS];
    unsigned count;
    uint32_t visible;
} BfmForestSwing;

/* Tick zero is the first pose of the grounded Fusion swing. The script marks
 * the two edges of its window. Actor contacts are resolved by the field. */
int bfm_forest_swing_load(const MusashiDiscMedia *disc, BfmForestSwing *out);
const BfmForestSwingFrame *bfm_forest_swing_frame(const BfmForestSwing *swing, unsigned tick);

#endif
