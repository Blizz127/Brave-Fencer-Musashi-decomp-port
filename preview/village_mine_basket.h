/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_BASKET_H
#define BFM_VILLAGE_MINE_BASKET_H

#include "forest_guard_contact.h"

typedef struct {
    int32_t angle_q16, velocity_q16;
    int ridden;
} BfmMineBasketSway;

/* One active update of the visible basket. Start with a zeroed state.
 * The invisible load-bearing child consumes this update's completed transform. */
int16_t bfm_mine_basket_step_sway(BfmMineBasketSway *sway);
void bfm_mine_basket_body_root(const BfmForestTransform *visual, BfmForestTransform *body);

/* Ridden is the physical support selected by the player's floor query.
 * Call once per active child update, including the first released field. */
void bfm_mine_basket_carry(BfmMineBasketSway *sway, const BfmForestGuardBox *box,
                           const BfmForestTransform *previous_body, const BfmForestTransform *body,
                           int ridden, int32_t foot_q16[3]);

typedef enum {
    BFM_MINE_BASKET_MISS,
    BFM_MINE_BASKET_KEEP,    /* Standing still, keep the old rider and foot. */
    BFM_MINE_BASKET_TOUCH,   /* Horizontal contact without a prior rider. */
    BFM_MINE_BASKET_BOARD,   /* Claim this basket and apply the returned foot. */
    BFM_MINE_BASKET_RIDE     /* Update the foot of an existing rider. */
} BfmMineBasketContact;

/* Run on one active basket after actor slot/exclusion selection. Pass the
 * candidate as previous when player movement sweeps are disabled. If this
 * basket had the rider, last_supported supplies its stored previous foot. */
BfmMineBasketContact bfm_mine_basket_probe(const BfmForestGuardBox *box,
    const BfmForestTransform *body, const int16_t previous[3], const int16_t candidate[3],
    const int16_t last_supported[3], int16_t foot[3]);

#endif
