/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_BELL_SOUND_H
#define BFM_TOWER_BELL_SOUND_H
#include "musashi_disc_media.h"

typedef struct BfmTowerBellSound BfmTowerBellSound;
/* Load the interior's sequence, effects, instrument mappings and samples from
 * the mounted US disc. Game thread commands must be serialized with mix. */
BfmTowerBellSound *bfm_tower_bell_sound_load(const MusashiDiscMedia *disc);
void bfm_tower_bell_sound_free(BfmTowerBellSound *sound);
void bfm_tower_bell_sound_command(BfmTowerBellSound *sound, unsigned command, unsigned argument);
/* Add the room's active dry voices to the 37,800 Hz stereo output. */
void bfm_tower_bell_sound_mix(BfmTowerBellSound *sound, int16_t *stereo, size_t frames);
#endif
