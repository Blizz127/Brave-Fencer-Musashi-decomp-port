/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_ABSORPTION_H
#define BFM_FOREST_ABSORPTION_H

#include "forest_field.h"
#include "forest_status.h"

typedef struct {
    unsigned slot, generation, kind, variant;
} BfmForestTetherTarget;

typedef enum {
    BFM_TETHER_IDLE,
    BFM_TETHER_ATTACHED,
    BFM_TETHER_EARNED,
    BFM_TETHER_LOST,
    BFM_TETHER_COMMITTED
} BfmForestTetherPhase;

typedef struct {
    BfmForestTetherPhase phase;
    BfmForestTetherTarget target;
    unsigned meter, ticks, decay_clock, gain, ability;
    uint32_t award_message, duration;
} BfmForestTether;

/* Pass the live actor actually struck by the thrown blade. The supported
 * encounter is the room's summoned gun soldier. Zero-initialize before use. */
int bfm_forest_tether_begin(BfmForestTether *tether, const BfmForestField *field,
                            const BfmForestTetherTarget *target);

/* Once per field simulation update, with the room's update counter and
 * player 1's fresh Square edge. A freed or replaced target breaks the tether.
 * Earning the mounted award starts its presentation; the status changes
 * only after the presenter closes and the returning blade finishes. */
BfmForestTetherPhase bfm_forest_tether_tick(BfmForestTether *tether,
        const BfmForestTetherTarget *target, unsigned room_tick,
        int square_pressed, int interrupted);

int bfm_forest_tether_commit(BfmForestTether *tether, BfmForestStatus *status);

#endif
