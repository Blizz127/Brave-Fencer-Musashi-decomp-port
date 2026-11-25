/* SPDX-License-Identifier: MIT */
#ifndef BFM_DESKTOP_VOICE_H
#define BFM_DESKTOP_VOICE_H

#include "musashi_disc_media.h"
#include <SDL.h>

typedef struct DesktopVoice DesktopVoice;
enum { BFM_VOICE_MESSAGE_READY = 1, BFM_VOICE_OUTPUT_STARTED = 2 };

/* All calls are made by the game thread. The mounted disc must outlive voice. */
DesktopVoice *bfm_desktop_voice_open(const MusashiDiscMedia *disc);
void bfm_desktop_voice_event(DesktopVoice *voice, const SDL_AudioDeviceEvent *event);
/* Returns one on a new decoder selection, zero for a repeat or unavailable clip. */
int bfm_desktop_voice_request(DesktopVoice *voice, unsigned bank, unsigned cue, unsigned field);
/* Scheduled scene field for the selected cue's message-ready event. */
unsigned bfm_desktop_voice_ready_field(const DesktopVoice *voice);
/* Returns the events that occurred on this opening field. The message becomes
 * ready before the first voice samples reach SDL. */
int bfm_desktop_voice_pump(DesktopVoice *voice, unsigned field, unsigned *bank, unsigned *cue);
struct ForestVoice;
/* Supply the already clocked forest transport to the same SDL output device. */
void bfm_desktop_voice_forest(DesktopVoice *voice, struct ForestVoice *source, unsigned generation);
void bfm_desktop_voice_stop(DesktopVoice *voice);
void bfm_desktop_voice_free(DesktopVoice *voice);
#endif
