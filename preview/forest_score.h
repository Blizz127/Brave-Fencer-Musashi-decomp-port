/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_SCORE_H
#define BFM_FOREST_SCORE_H

#include "musashi_disc_media.h"
typedef struct BfmForestScore BfmForestScore;

/* Prepare the mounted track and instruments on the game thread at 37800 or
 * 44100 Hz. The original disc may close after this call returns. */
BfmForestScore *bfm_forest_score_load(const MusashiDiscMedia *disc, unsigned rate);
void bfm_forest_score_rewind(BfmForestScore *score);
/* Add available stereo frames with saturation, retaining note and device
 * phase across calls and the mounted phrase return. Mixing allocates no
 * memory and performs no disc reads.
 * The caller serializes rewind, mix and free. */
size_t bfm_forest_score_mix(BfmForestScore *score, int16_t *stereo, size_t frames);
void bfm_forest_score_free(BfmForestScore *score);

#endif
