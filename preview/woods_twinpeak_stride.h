/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_TWINPEAK_STRIDE_H
#define BFM_WOODS_TWINPEAK_STRIDE_H
#include "bedroom_travel.h"
#include "village_ground.h"

/* Grounded scripted actors use the same summit floor as ordinary walking.
 * The actor retains each horizontal sub-unit remainder between updates. */
int bfm_twinpeak_turn(BfmBedroomRoot *root, unsigned heading);
int bfm_twinpeak_footing(const BfmVillageGround *ground, BfmBedroomRoot *root, int16_t to[3]);
int bfm_twinpeak_stride_target(const BfmBedroomRoot *root, int32_t position[3],
                              int32_t speed, unsigned yaw, int16_t to[3]);
int bfm_twinpeak_stride(const BfmVillageGround *ground, BfmBedroomRoot *root, int32_t position[3],
                        int32_t speed, unsigned yaw);
#endif
