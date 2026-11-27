/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_POLE_PICKUP_H
#define BFM_WOODS_POLE_PICKUP_H

#include "forest_status.h"
#include <stddef.h>

enum { BFM_WOODS_POLE_PICKUP_COUNT = 3 };

typedef struct {
    int16_t xyz[3];
    unsigned tier, amount, collected;
} BfmWoodsPolePickup;

typedef struct {
    BfmWoodsPolePickup items[BFM_WOODS_POLE_PICKUP_COUNT];
} BfmWoodsPolePickups;

/* Mount the three fixed BP pickups from the entering room's dynamic actor
 * list. Collection belongs to this visit and never changes the disc data. */
int bfm_woods_pole_pickups_load(BfmWoodsPolePickups *out, const uint8_t *scene, size_t bytes);

/* Check the ordinary player's post-motion position once per field. At most
 * one pickup is accepted per update, in room order. Return its sound cue and
 * nominal number even when the BP gain is capped. */
unsigned bfm_woods_pole_pickups_step(BfmWoodsPolePickups *pickups, BfmForestStatus *player,
                                    const int16_t feet[3], unsigned *popup_amount);

#endif
