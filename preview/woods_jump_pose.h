/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_JUMP_POSE_H
#define BFM_WOODS_JUMP_POSE_H

#include "bedroom_motion.h"
#include "forest_pose.h"

typedef struct {
    BfmForestJumpPoses standing, stride;
} BfmWoodsJumpPose;

/* The woods room supplies its mounted body-animation table. */
int bfm_woods_jump_pose_load(BfmWoodsJumpPose *out, const MusashiDiscMedia *disc,
                             uint32_t table);

/* The movement update keeps the takeoff gait through landing. phase_tick is
 * zero on the first update of each physical phase. main is MAIN/10's borrowed
 * type-0x101 payload; game flags still select the equipped model parts. */
int bfm_woods_jump_pose_apply(const BfmWoodsJumpPose *jump, int stride,
                              enum BfmForestJumpPhase phase, unsigned phase_tick,
                              const uint8_t *main, size_t main_bytes, BfmBedroomPose *body);

#endif
