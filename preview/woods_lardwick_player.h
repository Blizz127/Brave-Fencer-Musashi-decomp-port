/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_LARDWICK_PLAYER_H
#define BFM_WOODS_LARDWICK_PLAYER_H

#include "forest_pose.h"
#include "village_arrival_motion.h"

typedef struct {
    struct { unsigned command; int16_t x, y, z; } orders[2];
    unsigned count, walking, ready;
} BfmWoodsLardwickPlayer;

/* The walk target samples the Soldier's rendered transform. The turn samples
 * his physical position. Both orders retain the sampled targets. */
int bfm_woods_lardwick_player_queue(BfmWoodsLardwickPlayer *player, unsigned command,
    uint32_t argument, const BfmForestTransform *soldier_model, const int16_t soldier_xyz[3]);

/* Run once before the captive polls its script, while the approach owns player
 * movement. Clear player_ready while an earlier player action is completing.
 * ready stays clear through the field that completes the final order. */
int bfm_woods_lardwick_player_step(BfmWoodsLardwickPlayer *player, BfmVillageMotion *motion,
                                   int camera_yaw, int player_ready);

#endif
