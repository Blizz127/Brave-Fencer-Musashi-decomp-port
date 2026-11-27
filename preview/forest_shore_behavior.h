/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_SHORE_BEHAVIOR_H
#define BFM_FOREST_SHORE_BEHAVIOR_H

#include <stdint.h>

/* The four ordinary creek plants each have their own clock and facing. */
typedef struct {
    uint16_t state, tick, timer, outside_ticks;
    int16_t yaw;
    uint8_t noticed;
} BfmForestShoreBehavior;

enum {
    BFM_SHORE_FIGURE_RETIRE = 1,
    BFM_SHORE_FIGURE_ATTACK_CONTACT = 2,
    BFM_SHORE_FIGURE_NOTICE = 4
};

/* A new placement starts facing its recorded neutral orientation. */
void bfm_forest_shore_behavior_begin(BfmForestShoreBehavior *behavior);
/* On recovery the plant can watch again from its current position and yaw. */
void bfm_forest_shore_behavior_ready(BfmForestShoreBehavior *behavior);

/* Step one ordinary update. The owning actor suspends this clock during
 * a hit reaction and applies any returned retirement or attack event.
 * player is the current root used for retirement. target is the last
 * cached aim position and stays defined when targetable is zero. */
unsigned bfm_forest_shore_behavior_tick(BfmForestShoreBehavior *behavior,
                                        const int16_t at[3], const int16_t player[3],
                                        const int16_t target[3], int targetable);
uint32_t bfm_forest_shore_behavior_clip(const BfmForestShoreBehavior *behavior);

#endif
