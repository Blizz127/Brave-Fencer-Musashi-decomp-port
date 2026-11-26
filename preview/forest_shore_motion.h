/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_SHORE_MOTION_H
#define BFM_FOREST_SHORE_MOTION_H

#include "forest_field.h"

typedef enum {
    BFM_SHORE_WALK,
    BFM_SHORE_NOTICE
} BfmForestShorePhase;

typedef struct {
    int16_t position[3];
    int32_t y_q16;
    uint16_t yaw;
    unsigned frame;
    BfmForestShorePhase phase;
} BfmForestShoreMotion;

/* The room supplies its admitted far-shore placement. The first walking pose
 * is current on the birth tick. Subsequent updates run per game tick.
 * A failed update leaves the last supported state intact. */
int bfm_forest_shore_begin(BfmForestShoreMotion *motion, const BfmForestField *field,
                           const int16_t placement[3]);
/* The owner supplies the current mounted clip count after reading its wrap
 * marker. Wrapping the selected clip does not select a different action. */
int bfm_forest_shore_tick(BfmForestShoreMotion *motion, const BfmForestField *field,
                          const int16_t player[3], unsigned selected_frame_count);

#endif
