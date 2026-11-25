/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_POSE_H
#define BFM_FOREST_POSE_H
#include "musashi_forest_player_mesh.h"

typedef struct {
    int16_t rotation[3][3];
    int32_t translation[3];
} BfmForestTransform;

typedef struct {
    BfmForestTransform local[MUSASHI_FOREST_PLAYER_PARTS];
} BfmForestPose;

/* Load the frame selected by the live animation's pose cursor. Each local
 * transform is relative to the same model root. Pose and mesh can outlive
 * the disc and can be reused while the animation frame is held. */
int bfm_forest_pose_load(const MusashiDiscMedia *disc, uint32_t frame_address,
                         BfmForestPose *pose);
/* Room-owned and resident player poses share the same packed format. */
int bfm_forest_pose_decode(const uint8_t *bytes, size_t size, BfmForestPose *pose);
#endif
