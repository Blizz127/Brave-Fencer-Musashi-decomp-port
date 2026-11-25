/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_MANUAL_H
#define BFM_FOREST_MANUAL_H

#include "forest_field.h"

typedef struct {
    int moving;
    unsigned motion, elapsed;
    int have_origin;
    int16_t origin[3];
} BfmForestManual;

typedef enum {
    BFM_FOREST_MANUAL_ERROR = -1,
    BFM_FOREST_MANUAL_BOUNDARY = 0,
    BFM_FOREST_MANUAL_IDLE,
    BFM_FOREST_MANUAL_STARTED,
    BFM_FOREST_MANUAL_MOVING,
    BFM_FOREST_MANUAL_STOPPED
} BfmForestManualResult;

/* One effective player update on the flat doorway floor. Start with zeroed
 * state. Screen axes are digital -1, 0 or 1, with right and down positive.
 * While moving, motion and elapsed select the mounted locomotion pose.
 * A boundary or error preserves the last complete actor and animation state. */
BfmForestManualResult bfm_forest_manual_tick(BfmForestManual *state,
    const BfmForestField *field, ForestPlayerState *actor, const BfmForestCamera *camera,
    int screen_x, int screen_y, int16_t *yaw, int16_t cached[3]);

#endif
