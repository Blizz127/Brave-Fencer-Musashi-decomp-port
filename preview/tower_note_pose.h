/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_NOTE_POSE_H
#define BFM_TOWER_NOTE_POSE_H

#include "forest_speech_actor.h"

typedef struct {
    BfmForestSpeechActor actor;
    unsigned stage, pending_flags;
} BfmTowerNotePose;

/* Begin on the update that opens the Tower entrance message. Use the
 * destination room as the owner of the local gesture tracks. */
int bfm_tower_note_pose_begin(BfmTowerNotePose *note, const BfmForestField *tower_room,
                              const MusashiDiscMedia *disc, uint32_t parts, BfmForestPose *pose);
/* Pass the message actor_flags emitted on the previous update. Submit
 * actor.visible, blade_anchors and the returned pose with the held world root.
 * Emit the blade halo only while the held Fusion part (22) is visible.
 * After dialogue closes, the field's ordinary controller resumes. */
int bfm_tower_note_pose_tick(BfmTowerNotePose *note, unsigned actor_flags, BfmForestPose *pose);
#endif
