/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_IMPACT_SOUND_H
#define BFM_VILLAGE_IMPACT_SOUND_H

#include "musashi_disc_media.h"

typedef struct BfmVillageImpactSound BfmVillageImpactSound;

/* The entrance-5 landing in room 1011 requests one three-layer effect.
 * Start it once at contact. Serialize start and free with the audio mixer.
 * Mix adds to interleaved signed stereo at 37800 Hz. */
BfmVillageImpactSound *bfm_village_impact_sound_load(const MusashiDiscMedia *disc);
void bfm_village_impact_sound_free(BfmVillageImpactSound *sound);
void bfm_village_impact_sound_start(BfmVillageImpactSound *sound);
void bfm_village_impact_sound_mix(BfmVillageImpactSound *sound, int16_t *stereo, size_t frames);

#endif
