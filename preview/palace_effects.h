/* SPDX-License-Identifier: MIT */
#ifndef BFM_PALACE_EFFECTS_H
#define BFM_PALACE_EFFECTS_H
#include "musashi_disc_media.h"

typedef struct BfmPalaceEffects BfmPalaceEffects;
BfmPalaceEffects *bfm_palace_effects_load(const MusashiDiscMedia *disc);
void bfm_palace_effects_reset(BfmPalaceEffects *effects);
void bfm_palace_effects_mix(BfmPalaceEffects *effects, uint64_t opening_ms,
                            int16_t *stereo, size_t frames);
void bfm_palace_effects_free(BfmPalaceEffects *effects);
#endif
