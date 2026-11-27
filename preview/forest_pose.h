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

enum BfmForestJumpPhase {
    BFM_FOREST_JUMP_RISING, BFM_FOREST_JUMP_FALLING, BFM_FOREST_JUMP_LANDING
};

enum { BFM_FOREST_JUMP_TICKS = 32 };
typedef struct {
    uint32_t frames[3][BFM_FOREST_JUMP_TICKS];
    unsigned count[3], falling_loop;
    uint32_t visible;
} BfmForestJumpPoses;

/* Resolve the current Giant Forest player's mounted jump and recovery clips.
 * The game thread supplies ticks since the physical phase began. The last
 * rise key spans the first two falling updates. Ground contact immediately
 * selects the recovery clip. */
int bfm_forest_jump_poses_load(const MusashiDiscMedia *disc, uint32_t animation_table,
                               BfmForestJumpPoses *out);
uint32_t bfm_forest_jump_pose_frame(const BfmForestJumpPoses *poses,
                                    enum BfmForestJumpPhase phase, unsigned phase_tick);

/* The mounted stride clips serve a jump from moving ground and a walk-off.
 * A walk-off consumes one entry key per update from the loss of support,
 * then holds the last key until contact. */
int bfm_forest_walkoff_poses_load(const MusashiDiscMedia *disc, uint32_t animation_table,
                                  BfmForestJumpPoses *out);
uint32_t bfm_forest_walkoff_pose_frame(const BfmForestJumpPoses *poses, unsigned airborne_tick);

/* Load the frame selected by the live animation's pose cursor. Each local
 * transform is relative to the same model root. Pose and mesh can outlive
 * the disc and can be reused while the animation frame is held. */
int bfm_forest_pose_load(const MusashiDiscMedia *disc, uint32_t frame_address,
                         BfmForestPose *pose);
/* Room-owned and resident player poses share the same packed format. */
int bfm_forest_pose_decode(const uint8_t *bytes, size_t size, BfmForestPose *pose);
/* Apply the field model's level heading and integral world position. */
void bfm_forest_actor_root(unsigned yaw, const int16_t position[3], BfmForestTransform *root);
/* Apply pitch about the model's local X axis before its world heading. */
void bfm_forest_actor_pitch_yaw(unsigned pitch, unsigned yaw, const int16_t position[3],
                                BfmForestTransform *root);
/* Room actors have one packed local transform per mesh part, in mesh order. */
int bfm_forest_actor_pose_decode(const uint8_t *bytes, size_t size, unsigned part_count,
                                 BfmForestPose *pose);
/* Full room poses store signed 16-bit positions and rotations per part. */
int bfm_forest_actor_pose_decode_full(const uint8_t *frame, size_t bytes, unsigned count,
                                     BfmForestPose *pose);
#endif
