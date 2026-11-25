/* SPDX-License-Identifier: MIT */
#ifndef BFM_PALACE_VOICE_H
#define BFM_PALACE_VOICE_H

#include "musashi_disc_media.h"
#include <stddef.h>
#include <stdint.h>

enum { BFM_PALACE_VOICE_RATE = 37800, BFM_PALACE_VOICE_CHANNELS = 2 };
typedef struct PalaceVoice PalaceVoice;

/* Game-thread producer. Keep the disc alive until the voice is freed. */
PalaceVoice *bfm_palace_voice_load(const MusashiDiscMedia *disc);
/* Select an opening voice from its script request, rewinding it on success.
 * On failure the existing stream is preserved. */
int bfm_palace_voice_request(PalaceVoice *voice, unsigned bank, unsigned cue);
/* Return the number of decoded stereo frames. A short read ends the selected
 * recording. Unused output space is left alone. */
size_t bfm_palace_voice_read(PalaceVoice *voice, int16_t *stereo, size_t frames);
void bfm_palace_voice_stop(PalaceVoice *voice);
void bfm_palace_voice_free(PalaceVoice *voice);
#endif
