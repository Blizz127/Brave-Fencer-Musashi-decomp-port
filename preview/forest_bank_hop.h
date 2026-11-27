/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_BANK_HOP_H
#define BFM_FOREST_BANK_HOP_H

#include "forest_status.h"

typedef struct {
    int32_t vertical_q16;
    int16_t launch_y;
    uint8_t cost, active, cancelled_timer;
} BfmForestBankHop;

typedef enum {
    BFM_BANK_HOP_ORDINARY, BFM_BANK_HOP_MOVE,
    BFM_BANK_HOP_CANCEL
} BfmBankHopAction;

typedef struct {
    BfmBankHopAction action;
    int started;
    int32_t speed_q16, vertical_q16;
    unsigned yaw;
} BfmBankHopFrame;

/* Mount the automatic ability's price from the current SC03/6 catalog. */
int bfm_forest_bank_hop_init(BfmForestBankHop *hop, const uint8_t *scene, size_t bytes);
/* One eligible, unpaused player field. Acquisition and foreground actions have
 * their own dispatch and do not also run this movement. */
BfmBankHopFrame bfm_forest_bank_hop_step(BfmForestBankHop *hop, BfmForestStatus *player,
                                         int x, int y, int camera_yaw, unsigned facing,
                                         int circle_pressed);
/* On descending contact, start the next arc immediately at the supported foot. */
void bfm_forest_bank_hop_rebound(BfmForestBankHop *hop);
/* Bounce motion inside the tracking window leaves the camera at launch height. */
int16_t bfm_forest_bank_hop_camera_y(const BfmForestBankHop *hop, int16_t player_y);

#endif
