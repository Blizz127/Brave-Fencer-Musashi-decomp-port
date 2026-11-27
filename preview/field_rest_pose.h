/* SPDX-License-Identifier: MIT */
#ifndef BFM_FIELD_REST_POSE_H
#define BFM_FIELD_REST_POSE_H

#include "bedroom_motion.h"
#include "field_rest.h"

/* Advance the mounted field actor once. Tick zero enters a phase. Drop hands
 * off to Asleep on its mounted body marker. Wake selects ordinary stance. */
int bfm_bedroom_motion_field_rest(BfmBedroomMotion *motion, BfmFieldRestPose phase,
                                  unsigned tick, BfmBedroomPose *frame);

#endif
