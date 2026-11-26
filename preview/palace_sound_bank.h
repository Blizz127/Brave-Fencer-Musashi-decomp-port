/* SPDX-License-Identifier: MIT */
#ifndef BFM_PALACE_SOUND_BANK_H
#define BFM_PALACE_SOUND_BANK_H
#include "musashi_disc_media.h"

typedef struct BfmPalaceBank BfmPalaceBank;
typedef struct {
    const uint8_t *data;
    size_t bytes;
} BfmSpuSample;

/* Loads a paired instrument header and body from a palace scene member. */
BfmPalaceBank *bfm_palace_sound_bank_load(const MusashiDiscMedia *disc, unsigned member);
/* Sample numbers are one-based; the view is valid until the bank is freed. */
int bfm_palace_sound_sample(const BfmPalaceBank *bank, unsigned index, BfmSpuSample *sample);
void bfm_palace_sound_bank_free(BfmPalaceBank *bank);
#endif
