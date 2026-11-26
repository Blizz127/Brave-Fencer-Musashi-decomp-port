/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_TALK_CAMERA_H
#define BFM_VILLAGE_TALK_CAMERA_H

#include <stddef.h>
#include "village_camera.h"

typedef struct {
    int eye[3], target[3];
    int close_eye[3], close_target[3];
    int entry_eye[3], entry_target[3];
    unsigned phase, return_ticks;
} BfmVillageTalkCamera;

/* Initialize to zero on room entry. Begin once at the resident script's
 * camera cue. The supplied view is the last presented field camera. */
int bfm_village_talk_camera_begin(BfmVillageTalkCamera *talk, const uint8_t *scene,
                                  size_t bytes, const BfmVillageCamera *current);

/* Signal the return cue after the conversation's final wait. */
void bfm_village_talk_camera_release(BfmVillageTalkCamera *talk);

/* Once per 30 Hz update, after ordinary player-follow has computed its view. */
void bfm_village_talk_camera_step(BfmVillageTalkCamera *talk, BfmVillageCamera *normal);

#endif
