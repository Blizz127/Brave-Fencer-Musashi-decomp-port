/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_SHORE_MOTION_H
#define BFM_FOREST_SHORE_MOTION_H

#include "forest_field.h"

typedef enum {
    BFM_SHORE_WALK,
    BFM_SHORE_NOTICE,
    BFM_SHORE_RECOIL
} BfmForestShorePhase;

typedef struct {
    int16_t position[3];
    int32_t y_q16, recoil_vy_q16;
    uint16_t x_fraction, z_fraction;
    uint16_t yaw;
    unsigned frame, notice_delay;
    unsigned patrol_timer; /* Walking variant zero keeps the interval originally chosen at entry. */
    BfmForestShorePhase phase;
} BfmForestShoreMotion;

/* The room supplies its admitted far-shore placement. The first walking pose
 * is current on the birth tick. Subsequent updates run per game tick.
 * A failed update leaves the last supported state intact. */
int bfm_forest_shore_begin(BfmForestShoreMotion *motion, const BfmForestField *field,
                           const int16_t placement[3]);
unsigned bfm_forest_shore_bearing(int x, int z);
/* Integrate the first grounded hit on the flat bank. */
int bfm_forest_shore_recoil_height(BfmForestShoreMotion *motion, const BfmForestField *field);
/* The owner supplies the current mounted clip count. */
int bfm_forest_shore_tick(BfmForestShoreMotion *motion, const BfmForestField *field,
                          const int16_t player[3], unsigned selected_frame_count);

#endif
