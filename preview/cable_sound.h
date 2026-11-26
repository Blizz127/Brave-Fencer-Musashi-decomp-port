/* SPDX-License-Identifier: MIT */
#ifndef BFM_CABLE_SOUND_H
#define BFM_CABLE_SOUND_H

#include "musashi_disc_media.h"

typedef struct BfmCableSound BfmCableSound;

/* The entrance-0 slider starts its mounted sequence once. Room command 28
 * begins its release. Serialize these operations with the 37800 Hz mixer. */
BfmCableSound *bfm_cable_sound_load(const MusashiDiscMedia *disc);
void bfm_cable_sound_free(BfmCableSound *sound);
void bfm_cable_sound_start(BfmCableSound *sound);
void bfm_cable_sound_release(BfmCableSound *sound);
void bfm_cable_sound_mix(BfmCableSound *sound, int16_t *stereo, size_t frames);
#endif
