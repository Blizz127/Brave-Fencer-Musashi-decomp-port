/* SPDX-License-Identifier: MIT */
#ifndef BFM_DISC_VOICE_H
#define BFM_DISC_VOICE_H

#include "musashi_disc_media.h"

/* Both scene interfaces can share the original opaque C type. */
typedef struct PalaceVoice DiscVoice;
typedef enum { BFM_VOICE_OPEN, BFM_VOICE_ST01, BFM_VOICE_ST02, BFM_VOICE_SHOPS01, BFM_VOICE_OUT,
               BFM_VOICE_ST03 } BfmDiscVoiceArchive;
typedef struct {
    unsigned channel_offset, sound_ticks, group_head_frames;
} BfmDiscVoiceCue;

/* Shares the immutable mount. Successful requests rewind the selected
 * recording; invalid requests preserve the current decoder and its history. */
DiscVoice *bfm_disc_voice_load(const MusashiDiscMedia *disc, BfmDiscVoiceArchive archive);
int bfm_disc_voice_request(DiscVoice *voice, unsigned bank, unsigned cue, BfmDiscVoiceCue *selected);
/* Load the next stereo block without consuming its first sample. */
int bfm_disc_voice_prime(DiscVoice *voice);
size_t bfm_disc_voice_read(DiscVoice *voice, int16_t *stereo, size_t frames);
void bfm_disc_voice_stop(DiscVoice *voice);
void bfm_disc_voice_free(DiscVoice *voice);
#endif
