/* SPDX-License-Identifier: MIT */
#include "palace_voice.h"
#include "disc_voice.h"

PalaceVoice *bfm_palace_voice_load(const MusashiDiscMedia *disc)
{
    return bfm_disc_voice_load(disc, BFM_VOICE_OPEN);
}

int bfm_palace_voice_request(PalaceVoice *voice, unsigned bank, unsigned cue)
{
    return bfm_disc_voice_request(voice, bank, cue, NULL);
}

size_t bfm_palace_voice_read(PalaceVoice *voice, int16_t *stereo, size_t frames)
{
    return bfm_disc_voice_read(voice, stereo, frames);
}

void bfm_palace_voice_stop(PalaceVoice *voice)
{
    bfm_disc_voice_stop(voice);
}

void bfm_palace_voice_free(PalaceVoice *voice)
{
    bfm_disc_voice_free(voice);
}
