/* SPDX-License-Identifier: MIT */
#ifndef BFM_VALLEY_ROCK_FALL_H
#define BFM_VALLEY_ROCK_FALL_H

#include "village_ground.h"
#include <stdint.h>

typedef struct {
    int32_t velocity[3], acceleration[3];
} BfmValleyRockFall;

/* The update after a normal rock leaves the cliff sets its bearing and
 * acceleration. The first displacement occurs on the following update. */
void bfm_valley_rock_fall_begin(BfmValleyRockFall *fall, const int32_t position_q16[3],
                                const int16_t landing[3]);

/* Use the mounted model's probe offset. A gentle-floor hit settles the rock
 * at the corrected position and returns 1. The caller owns the settled flag. */
int bfm_valley_rock_fall_step(BfmValleyRockFall *fall, const BfmVillageGround *ground,
                              const int16_t probe_offset[3], int32_t position_q16[3]);

#endif
