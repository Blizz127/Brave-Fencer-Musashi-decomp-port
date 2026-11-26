/* SPDX-License-Identifier: MIT */
#include "tower_note_pose.h"

enum { WAIT_LOOK, LOOKING, WAIT_NOTE, DRAWING_NOTE, READING, PUTTING_AWAY, FINISHED };

int bfm_tower_note_pose_begin(BfmTowerNotePose *note, const BfmForestField *tower_room,
                              const MusashiDiscMedia *disc, uint32_t parts, BfmForestPose *pose)
{
    if (!note) return 0;
    BfmTowerNotePose fresh = {0};
    if (!bfm_forest_speech_actor_begin(&fresh.actor, tower_room, disc, parts, pose)) return 0;
    *note = fresh;
    return 1;
}

int bfm_tower_note_pose_tick(BfmTowerNotePose *note, unsigned actor_flags, BfmForestPose *pose)
{
    if (!note || !note->actor.field || !pose) return 0;
    note->pending_flags |= actor_flags & 7u;
    uint32_t script = 0;
    switch (note->stage) {
    case WAIT_LOOK:
        if (note->pending_flags & 1u) {
            script = UINT32_C(0x800d599c);
            note->stage = LOOKING;
        }
        break;
    case LOOKING:
    case PUTTING_AWAY:
        if (note->actor.complete) {
            script = UINT32_C(0x800d5a34);
            note->stage = note->stage == LOOKING ? WAIT_NOTE : FINISHED;
        }
        break;
    case WAIT_NOTE:
        if (note->pending_flags & 2u) {
            script = UINT32_C(0x801894cc);
            note->stage = DRAWING_NOTE;
        }
        break;
    case DRAWING_NOTE:
    case READING:
        if (note->pending_flags & 4u) {
            script = UINT32_C(0x8018954c);
            note->stage = PUTTING_AWAY;
        } else if (note->stage == DRAWING_NOTE && note->actor.complete) {
            script = UINT32_C(0x801895cc);
            note->stage = READING;
        }
        break;
    case FINISHED:
        break;
    default:
        return 0;
    }
    return bfm_forest_speech_motion_tick(&note->actor, script, pose);
}
