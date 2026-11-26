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
typedef enum {
    BFM_FOREST_STANDING_CONTINUE,
    BFM_FOREST_STANDING_STOP,
    BFM_FOREST_STANDING_RUN_STOP
} BfmForestStandingEvent;

/* Select or advance one mounted standing frame after the conversation. */
int bfm_forest_standing_tick(BfmForestSpeechActor *actor, BfmForestStandingEvent event,
                             const ForestPlayerState *player, BfmForestPose *pose);
/* A nonzero return is the local forward velocity of the current release pose.
 * The selection update sets the local velocity without integrating it. */
int32_t bfm_forest_standing_forward_q16(const BfmForestSpeechActor *actor);
#endif
