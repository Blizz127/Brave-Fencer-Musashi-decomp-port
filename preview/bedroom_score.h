/* SPDX-License-Identifier: MIT */
#ifndef BFM_BEDROOM_SCORE_H
#define BFM_BEDROOM_SCORE_H

#include "forest_score.h"
int bfm_bedroom_resident_data(const MusashiDiscMedia *disc, uint32_t address, uint8_t *out, size_t size);
BfmForestScore *bfm_bedroom_score_load(const MusashiDiscMedia *disc, unsigned selector);
#endif
