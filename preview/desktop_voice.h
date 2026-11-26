/* SPDX-License-Identifier: MIT */
#ifndef BFM_DESKTOP_VOICE_H
#define BFM_DESKTOP_VOICE_H

#include "musashi_disc_media.h"
#include <SDL.h>

typedef struct DesktopVoice DesktopVoice;
enum { BFM_VOICE_MESSAGE_READY = 1, BFM_VOICE_OUTPUT_STARTED = 2 };

/* All calls are made by the game thread. The mounted disc must outlive voice. */
DesktopVoice *bfm_desktop_voice_open(const MusashiDiscMedia *disc);
/* The opening and its instrument track share this SDL millisecond origin. */
void bfm_desktop_voice_opening(DesktopVoice *voice, Uint64 start);
void bfm_desktop_voice_event(DesktopVoice *voice, const SDL_AudioDeviceEvent *event);
/* Returns one on a new decoder selection, zero for a repeat or unavailable clip. */
int bfm_desktop_voice_request(DesktopVoice *voice, unsigned bank, unsigned cue, unsigned field);
/* Scheduled scene field for the selected cue's message-ready event. */
unsigned bfm_desktop_voice_ready_field(const DesktopVoice *voice);
/* Returns the events that occurred on this opening field. The message becomes
 * ready before the first voice samples reach SDL. */
int bfm_desktop_voice_pump(DesktopVoice *voice, unsigned field, unsigned *bank, unsigned *cue);
/* Arm the mounted incoming score at its opening-clock SDL time. Repeated
 * frames and later dialogue generations retain the same music cursor. */
void bfm_desktop_voice_forest_score(DesktopVoice *voice, const MusashiDiscMedia *disc, Uint64 start);
/* Play the mounted common-bank sound for an accepted first Fusion swing. */
void bfm_desktop_voice_forest_swing(DesktopVoice *voice);
/* A separate area-bank effect plays when the guard actually takes a hit. */
void bfm_desktop_voice_forest_hit(DesktopVoice *voice);
/* Invoke on the earned skill's firing marker once the shot spends BP. */
void bfm_desktop_voice_forest_gunshot(DesktopVoice *voice);
struct BfmTowerBellSound;
void bfm_desktop_voice_tower_bell(DesktopVoice *voice, struct BfmTowerBellSound *bell,
                                  unsigned command, unsigned argument);
struct BfmVillageImpactSound;
void bfm_desktop_voice_landing(DesktopVoice *voice, struct BfmVillageImpactSound *sound);
struct BfmCableSound;
void bfm_desktop_voice_slider(DesktopVoice *voice, struct BfmCableSound *slider, int event);
struct BfmBedroomEffects;
void bfm_desktop_voice_room_effect(DesktopVoice *voice, struct BfmBedroomEffects *effects, unsigned command);
struct BfmForestScore;
/* The active room retains its score until the desktop stops. */
void bfm_desktop_voice_room_skip(DesktopVoice *voice);
void bfm_desktop_voice_room_score(DesktopVoice *voice, struct BfmForestScore *score);
struct ForestVoice;
/* Supply the clocked field transport to SDL. Giant Forest provides its NTSC
 * field so complete lines can drain on a slow device. Zero selects immediate
 * replacement for rooms without that timing contract. */
void bfm_desktop_voice_forest(DesktopVoice *voice, struct ForestVoice *source, unsigned generation, unsigned room_field);
/* Finish the already-scheduled samples up to the final scene field. */
void bfm_desktop_voice_finish(DesktopVoice *voice);
void bfm_desktop_voice_stop(DesktopVoice *voice);
void bfm_desktop_voice_free(DesktopVoice *voice);
#endif
