/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_BANK_HOP_POSE_H
#define BFM_FOREST_BANK_HOP_POSE_H

#include "bedroom_motion.h"
#include "musashi_disc_media.h"

typedef struct BfmForestBankHopPose BfmForestBankHopPose;

/* The mounted Hop resource supplies both clips and every body packet. */
BfmForestBankHopPose *bfm_forest_bank_hop_pose_open(const MusashiDiscMedia *disc);
void bfm_forest_bank_hop_pose_free(BfmForestBankHopPose *poses);

/* Reset on leaving the ability. Only advance on fields that update the hero.
 * takeoff is true on the first bounce and on each subsequent rebound.
 * actor_flags carries the live actor's detached Fusion mask (4).
 * Pass face_already_stepped if locomotion already updated the ordinary face
 * this field. The returned body's mounted pose is valid until free. */
void bfm_forest_bank_hop_pose_reset(BfmForestBankHopPose *poses);
int bfm_forest_bank_hop_pose_step(BfmForestBankHopPose *poses, int takeoff, uint32_t actor_flags,
                                 BfmBedroomMotion *ordinary, int face_already_stepped,
                                 BfmBedroomPose *body);

#endif
