/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_SPEECH_ACTOR_H
#define BFM_FOREST_SPEECH_ACTOR_H
#include "forest_field.h"
#include "../pc_port/forest_player_archive.h"

typedef enum {
    BFM_FOREST_TALK_START, BFM_FOREST_TALK_FIRST, BFM_FOREST_TALK_SECOND,
    BFM_FOREST_TALK_END_GESTURE, BFM_FOREST_TALK_RELEASED
} BfmForestTalkPhase;

typedef struct {
    const BfmForestField *field;
    ForestPlayerArchive player;
    BfmForestTalkPhase phase;
    uint32_t script, frame, visible, entry_visible;
    int8_t blade_anchors[2][3];
    unsigned cursor, standing_step, standing_cycles;
    int complete;
} BfmForestSpeechActor;

int bfm_forest_speech_actor_begin(BfmForestSpeechActor *actor, const BfmForestField *field,
                                  const MusashiDiscMedia *disc, uint32_t parts,
                                  BfmForestPose *pose);
/* The two authored message events select gestures. The second gesture's
 * animation endpoint advances the actor again. Closing the message releases it. */
int bfm_forest_speech_actor_tick(BfmForestSpeechActor *actor, int first, int second,
                                 int closed, BfmForestPose *pose);
/* Run once per stationary actor update after the conversation. The first
 * stationary update following movement starts a fresh return gesture. */
int bfm_forest_standing_tick(BfmForestSpeechActor *actor, int newly_stopped,
                             const ForestPlayerState *player, BfmForestPose *pose);
#endif
