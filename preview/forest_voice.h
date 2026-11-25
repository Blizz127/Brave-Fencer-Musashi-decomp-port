/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_VOICE_H
#define BFM_FOREST_VOICE_H

#include "musashi_disc_media.h"

typedef struct ForestVoice ForestVoice;
enum { BFM_FOREST_VOICE_IDLE = 0, BFM_FOREST_VOICE_PENDING = 1,
       BFM_FOREST_VOICE_PLAYING = 4, BFM_FOREST_VOICE_RATE = 37800 };

/* A game-thread service. The mounted image must outlive the voice. */
ForestVoice *bfm_forest_voice_load(const MusashiDiscMedia *disc);
int bfm_forest_voice_request(ForestVoice *voice, unsigned bank, unsigned cue);
/* Advance once per two NTSC fields, before the forest script checks status. */
void bfm_forest_voice_step(ForestVoice *voice);
unsigned bfm_forest_voice_status(const ForestVoice *voice);
/* Frames of native-rate signed 16-bit interleaved stereo. Read only while
 * PLAYING. The title's sound-tick deadline bounds the recording supplied. */
size_t bfm_forest_voice_read(ForestVoice *voice, int16_t *stereo, size_t frames);
void bfm_forest_voice_stop(ForestVoice *voice);
void bfm_forest_voice_free(ForestVoice *voice);
#endif
