/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_MANUAL_H
#define BFM_FOREST_MANUAL_H

#include "forest_field.h"

typedef struct {
    int moving, after_jump, left_support;
    int32_t last_descent;
    unsigned motion, elapsed;
    int16_t ground_normal[3];
} BfmForestManual;

typedef enum {
    BFM_FOREST_MANUAL_ERROR = -1,
    BFM_FOREST_MANUAL_BOUNDARY = 0,
    BFM_FOREST_MANUAL_IDLE,
    BFM_FOREST_MANUAL_STARTED,
    BFM_FOREST_MANUAL_MOVING,
    BFM_FOREST_MANUAL_STOPPED,
    BFM_FOREST_MANUAL_RUN_STOPPED,
    BFM_FOREST_MANUAL_FALL
} BfmForestManualResult;

/* A launch consumes the rising Cross edge. The held level on subsequent
 * updates controls how high the actor can rise. Elapsed starts at zero.
 * A positive landing result retains the contact until the recovery finishes. */
typedef struct {
    unsigned active, elapsed, neutral_ticks, descending, landed, recovery, phase_elapsed;
    int steering, walk_off;
    int32_t touchdown_velocity;
    BfmForestContact ground;
} BfmForestJump;

typedef enum {
    BFM_FOREST_JUMP_BOUNDARY,
    BFM_FOREST_JUMP_AIRBORNE,
    BFM_FOREST_JUMP_LANDED,
    BFM_FOREST_JUMP_FINISHED
} BfmForestJumpResult;

int bfm_forest_jump_begin(BfmForestJump *jump, const BfmForestField *field,
                          ForestPlayerState *actor, const int16_t cached[3]);
/* Accept the preceding ground update's loss of support. The same update
 * that takes ownership advances horizontal movement with jump_tick. */
int bfm_forest_fall_begin(BfmForestJump *jump, BfmForestManual *manual,
                          ForestPlayerState *actor);
BfmForestJumpResult bfm_forest_jump_tick(BfmForestJump *jump, const BfmForestField *field,
    ForestPlayerState *actor, const BfmForestCamera *camera, int x, int y, int cross_held,
    int16_t *yaw, int16_t cached[3]);

/* Install the landing state on the transition update. Directed movement then
 * resumes on the next ground update with the jump's retained momentum. */
void bfm_forest_jump_resume(BfmForestManual *manual, const BfmForestJump *jump,
                            const ForestPlayerState *actor);

/* One effective player update in the entrance corridor. Start with zeroed
 * state. Screen axes are digital -1, 0 or 1, with right and down positive.
 * While moving, motion and elapsed select the mounted locomotion pose.
 * A boundary or error preserves the last complete actor and animation state. */
BfmForestManualResult bfm_forest_manual_tick(BfmForestManual *state,
    const BfmForestField *field, ForestPlayerState *actor, const BfmForestCamera *camera,
    int screen_x, int screen_y, int16_t *yaw, int16_t cached[3]);


/* Integrate the current standing motion's local velocity on the room floor. */
int bfm_forest_manual_coast(const BfmForestField *field, ForestPlayerState *actor,
                            int16_t yaw, int16_t cached[3]);

#endif
