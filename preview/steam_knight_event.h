/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_EVENT_H
#define BFM_STEAM_KNIGHT_EVENT_H
#include "forest_voice.h"
#include "gameplay_run.h"
#include "room_descriptor.h"
typedef enum {
    BFM_STEAM_ARRIVAL, BFM_STEAM_ENTER_HALL, BFM_STEAM_EXCHANGE, BFM_STEAM_APPROACH,
    BFM_STEAM_ENCLOSURE, BFM_STEAM_INSTRUCTION, BFM_STEAM_READY,
    BFM_STEAM_ROTATING, BFM_STEAM_REPLY, BFM_STEAM_AFTER_REPLY
} BfmSteamStage;
typedef enum {
    BFM_STEAM_ENTRY_CAMERA, BFM_STEAM_CAPTOR_CAMERA, BFM_STEAM_HALL_CAMERA,
    BFM_STEAM_CAPTOR_LOW_CAMERA, BFM_STEAM_FIRE_CAMERA, BFM_STEAM_PRINCESS_CAMERA,
    BFM_STEAM_TUTORIAL_CAMERA, BFM_STEAM_REPLY_CAMERA
} BfmSteamShot;
typedef struct {
    BfmSteamStage stage;
    BfmSteamShot shot;
    int16_t position[3], heading;
    unsigned age, stage_age, message_age, step, wait, grip, voice_generation;
    const uint8_t *programs;
    BfmGameplayRun *run;
    ForestVoice *voice;
    BfmForestMessage *message;
} BfmSteamKnightEvent;
/* The room, voice service and live player are borrowed from the owner. */
int bfm_steam_event_begin(BfmSteamKnightEvent *, const uint8_t *scene, size_t bytes,
                          const RoomTransferDescriptor *, BfmGameplayRun *, ForestVoice *);
void bfm_steam_event_clear(BfmSteamKnightEvent *);
int bfm_steam_event_tick(BfmSteamKnightEvent *, uint16_t pressed, uint16_t held, unsigned live_flames);
int bfm_steam_event_captor_gesture(const BfmSteamKnightEvent *);
int bfm_steam_event_ring_started(const BfmSteamKnightEvent *);
int bfm_steam_event_hud(const BfmSteamKnightEvent *);
unsigned bfm_steam_event_shade(const BfmSteamKnightEvent *);
#endif
