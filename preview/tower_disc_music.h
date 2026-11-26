/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_DISC_MUSIC_H
#define BFM_TOWER_DISC_MUSIC_H

#include "musashi_disc_media.h"

typedef struct BfmTowerDiscMusic BfmTowerDiscMusic;
/* Stage one of the two summit retry tracks from the mounted disc. */
BfmTowerDiscMusic *bfm_tower_disc_music_load(const MusashiDiscMedia *disc, unsigned entrance);
void bfm_tower_disc_music_fade(BfmTowerDiscMusic *music);
/* Add signed stereo at the desktop's 37,800 Hz. The selected track plays once. */
size_t bfm_tower_disc_music_mix(BfmTowerDiscMusic *music, int16_t *out, size_t frames);
void bfm_tower_disc_music_free(BfmTowerDiscMusic *music);
#endif
