/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_EVENT_H
#define BFM_STEAM_KNIGHT_EVENT_H
#include "forest_voice.h"
#include "gameplay_run.h"
#include "room_descriptor.h"
typedef enum {
    BFM_STEAM_ARRIVAL, BFM_STEAM_ENTER_HALL, BFM_STEAM_EXCHANGE, BFM_STEAM_APPROACH,
    BFM_STEAM_ENCLOSURE, BFM_STEAM_INSTRUCTION, BFM_STEAM_READY,
    BFM_STEAM_ROTATING, BFM_STEAM_REPLY, BFM_STEAM_HOSTAGE_PRAISE, BFM_STEAM_SILENCE,
    BFM_STEAM_FAREWELL, BFM_STEAM_CHASE, BFM_STEAM_RUMBLE, BFM_STEAM_LOOK_UP,
    BFM_STEAM_BREACH, BFM_STEAM_DESCENT, BFM_STEAM_CHALLENGE, BFM_STEAM_DUEL
} BfmSteamStage;
typedef enum {
    BFM_STEAM_ENTRY_CAMERA, BFM_STEAM_CAPTOR_CAMERA, BFM_STEAM_HALL_CAMERA,
    BFM_STEAM_CAPTOR_LOW_CAMERA, BFM_STEAM_FIRE_CAMERA, BFM_STEAM_PRINCESS_CAMERA,
    BFM_STEAM_TUTORIAL_CAMERA, BFM_STEAM_REPLY_CAMERA, BFM_STEAM_CARPET_CAMERA,
    BFM_STEAM_ROOF_CAMERA, BFM_STEAM_MACHINE_CAMERA, BFM_STEAM_DUEL_CAMERA
} BfmSteamShot;
typedef struct {
    BfmSteamStage stage;
    BfmSteamShot shot;
    int16_t position[3], heading;
    unsigned age, stage_age, message_age, step, wait, grip, voice_generation;
    unsigned landing_age;
    const uint8_t *programs;
    size_t program_bytes;
    BfmGameplayRun *run;
    ForestVoice *voice;
    BfmForestMessage *message;
} BfmSteamKnightEvent;
typedef enum {
    BFM_STEAM_FINAL_THROW_TAUNT, BFM_STEAM_FINAL_RECOLLECTION
} BfmSteamFinalSpeech;
/* The mounted room must outlive the returned window. The caller owns it. */
BfmForestMessage *bfm_steam_event_final_speech(const BfmSteamKnightEvent *, BfmSteamFinalSpeech);
/* The room, voice service and live player are borrowed from the owner. */
int bfm_steam_event_begin(BfmSteamKnightEvent *, const uint8_t *scene, size_t bytes,
                          const RoomTransferDescriptor *, BfmGameplayRun *, ForestVoice *);
void bfm_steam_event_clear(BfmSteamKnightEvent *);
int bfm_steam_event_tick(BfmSteamKnightEvent *, uint16_t pressed, uint16_t held, unsigned live_flames);
void bfm_steam_event_captor_pose(const BfmSteamKnightEvent *, unsigned *clip, unsigned *age);
int bfm_steam_event_show_captors(const BfmSteamKnightEvent *);
int bfm_steam_event_title(const BfmSteamKnightEvent *);
int bfm_steam_event_ring_started(const BfmSteamKnightEvent *);
int bfm_steam_event_hud(const BfmSteamKnightEvent *);
unsigned bfm_steam_event_shade(const BfmSteamKnightEvent *);
#endif
