/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_SHORE_POSE_H
#define BFM_FOREST_SHORE_POSE_H

#include "forest_field.h"

/* Decode the near-shore actor's local pose from the mounted forest. Tick
 * zero selects the first key on entry to the supplied actor state. States
 * 1 and 3 loop; state 2 is a single twenty-update transition. */
int bfm_forest_shore_pose(const BfmForestField *field, unsigned state,
                          unsigned state_tick, BfmForestPose *out);

/* Decode a key of the mounted shore model, including its response clips. */
int bfm_forest_shore_clip_pose(const BfmForestField *field, uint32_t clip_address,
                               unsigned index, BfmForestPose *out);

#endif
