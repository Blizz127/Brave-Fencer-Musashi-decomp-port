/* SPDX-License-Identifier: MIT */
#ifndef BFM_PALACE_SOUND_BANK_H
#define BFM_PALACE_SOUND_BANK_H
#include "musashi_disc_media.h"

typedef struct BfmPalaceBank BfmPalaceBank;
typedef struct {
    const uint8_t *data;
    size_t bytes;
} BfmSpuSample;

enum { BFM_PALACE_MAX_LAYERS = 16 };
typedef struct {
    unsigned sample, key_low, key_high, center;
    int tuning;
    unsigned bank_gain, bank_pan, program_gain, program_pan, tone_gain, tone_pan;
    unsigned priority, mode, adsr1, adsr2;
    /* Mounted range bytes in stored order. Both agree on every forest tone. */
    unsigned bend_range[2];
} BfmPalaceTone;

/* SC01 scenes carry paired effect banks. The palace (2) and Giant Forest
 * (78) score members each carry their own instrument bank. */
BfmPalaceBank *bfm_palace_sound_bank_load(const MusashiDiscMedia *disc, unsigned member);
BfmPalaceBank *bfm_archive_sound_bank_load(const MusashiDiscMedia *disc, const char *archive, unsigned member);
/* Sample numbers are one-based; the view is valid until the bank is freed. */
int bfm_palace_sound_sample(const BfmPalaceBank *bank, unsigned index, BfmSpuSample *sample);
int bfm_room_effect_tone(const BfmPalaceBank *bank, unsigned program, unsigned layer, BfmPalaceTone *tone);
/* Returns every matching tone in mounted order. The caller supplies room for 16. */
unsigned bfm_palace_sound_tones(const BfmPalaceBank *bank, unsigned program, unsigned key,
                                BfmPalaceTone tones[BFM_PALACE_MAX_LAYERS]);
void bfm_palace_sound_bank_free(BfmPalaceBank *bank);
#endif
