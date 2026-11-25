/* SPDX-License-Identifier: MIT */
#ifndef BFM_DESKTOP_VOICE_H
#define BFM_DESKTOP_VOICE_H

#include "musashi_disc_media.h"
#include <SDL.h>

typedef struct DesktopVoice DesktopVoice;

/* All calls are made by the game thread. The mounted disc must outlive voice. */
DesktopVoice *bfm_desktop_voice_open(const MusashiDiscMedia *disc);
void bfm_desktop_voice_event(DesktopVoice *voice, const SDL_AudioDeviceEvent *event);
/* Returns one on a new decoder selection, zero for a repeat or unavailable clip. */
int bfm_desktop_voice_request(DesktopVoice *voice, unsigned bank, unsigned cue);
/* Reports a cue once SDL has pulled its first PCM frames for the open output.
 * SDL2 does not expose the presentation time at the physical speakers. */
int bfm_desktop_voice_pump(DesktopVoice *voice, unsigned *bank, unsigned *cue);
void bfm_desktop_voice_stop(DesktopVoice *voice);
void bfm_desktop_voice_free(DesktopVoice *voice);
#endif
