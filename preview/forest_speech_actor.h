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
    unsigned cursor, standing_step, standing_cycles, face_slot;
    int complete;
} BfmForestSpeechActor;

int bfm_forest_speech_actor_begin(BfmForestSpeechActor *actor, const BfmForestField *field,
                                  const MusashiDiscMedia *disc, uint32_t parts,
                                  BfmForestPose *pose);
/* Advance a dialogue actor on its current mounted motion. A nonzero script
 * changes the motion before this update's ordinary animation service. */
int bfm_forest_speech_motion_tick(BfmForestSpeechActor *actor, uint32_t script,
                                  BfmForestPose *pose);
/* The two authored message events select gestures. The second gesture's
 * animation endpoint advances the actor again. Closing the message releases it. */
int bfm_forest_speech_actor_tick(BfmForestSpeechActor *actor, int first, int second,
                                 int closed, BfmForestPose *pose);
typedef enum {
    BFM_FOREST_CLEARING_CONTINUE,
    BFM_FOREST_CLEARING_CLIFF,
    BFM_FOREST_CLEARING_RECOVER,
    BFM_FOREST_CLEARING_FACE_SOLDIER
} BfmForestClearingEvent;

/* Advance the existing forest hero once per game update while the bridge
 * event owns control. CLIFF opens with the first speech; RECOVER follows
 * its dismissal. FACE_SOLDIER restores the stance when discovery opens. */
int bfm_forest_clearing_pose_tick(BfmForestSpeechActor *actor, BfmForestClearingEvent event,
                                  BfmForestPose *pose);

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
