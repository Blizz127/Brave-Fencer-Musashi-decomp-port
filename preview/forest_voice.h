/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_VOICE_H
#define BFM_FOREST_VOICE_H

#include "musashi_disc_media.h"
#include "forest_message.h"

typedef struct ForestVoice ForestVoice;
enum { BFM_FOREST_VOICE_IDLE = 0, BFM_FOREST_VOICE_PENDING = 1,
       BFM_FOREST_VOICE_PLAYING = 4, BFM_FOREST_VOICE_RATE = 37800 };

/* A game-thread service. The mounted image must outlive the voice. */
ForestVoice *bfm_forest_voice_load(const MusashiDiscMedia *disc);
ForestVoice *bfm_bedroom_voice_load(const MusashiDiscMedia *disc);
ForestVoice *bfm_mine_gate_voice_load(const MusashiDiscMedia *disc);
ForestVoice *bfm_inn_voice_load(const MusashiDiscMedia *disc);
int bfm_forest_voice_request(ForestVoice *voice, unsigned bank, unsigned cue);
/* Advance once per two NTSC fields, before the forest script checks status. */
void bfm_forest_voice_step(ForestVoice *voice);
unsigned bfm_forest_voice_status(const ForestVoice *voice);
/* For ST02, the group's first sector can be present before continuous
 * playback. A different selected channel has no samples in that sector. */
int bfm_bedroom_voice_primed_frames(const ForestVoice *voice, size_t *frames);
/* Frames of native-rate signed 16-bit interleaved stereo. Read only while
 * PLAYING. The title's sound-tick deadline bounds the recording supplied. */
size_t bfm_forest_voice_read(ForestVoice *voice, int16_t *stereo, size_t frames);
void bfm_forest_voice_stop(ForestVoice *voice);
ForestVoice *bfm_defeat_voice_load(const MusashiDiscMedia *disc);
ForestVoice *bfm_library_voice_load(const MusashiDiscMedia *disc);
/* Tick the page against the mounted voice service and deliver its requests. */
const BfmForestMessageFrame *bfm_forest_voice_page(ForestVoice *voice, BfmForestMessage *page,
                                                   unsigned *generation, int confirm, int reveal);
void bfm_forest_voice_free(ForestVoice *voice);
#endif
