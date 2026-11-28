/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_POLE_CLIMB_H
#define BFM_WOODS_POLE_CLIMB_H

#include <stddef.h>
#include <stdint.h>
#include "field_input.h"

enum { BFM_WOODS_POLE_COUNT = 3 };

typedef struct {
    unsigned count;
    int16_t origin[BFM_WOODS_POLE_COUNT][3];
    int16_t minimum[3], maximum[3]; /* Relative to each upright pole. */
} BfmWoodsPoleLayout;

typedef struct {
    unsigned pole; /* One-based row in layout, zero while free. */
    unsigned settling, regrab, previous;
} BfmWoodsPoleGrip;

typedef enum {
    BFM_POLE_FREE, BFM_POLE_HOLD, BFM_POLE_ASCEND, BFM_POLE_DESCEND,
    BFM_POLE_JUMP, BFM_POLE_FALL
} BfmWoodsPoleMove;

/* Reads SC02 member 11, expanded section zero. A failed load preserves layout. */
int bfm_woods_pole_load(BfmWoodsPoleLayout *layout, const uint8_t *scene, size_t bytes);

/* Reads the single stationary grip in SC03 member 107, expanded section zero. */
int bfm_mine_pole_load(BfmWoodsPoleLayout *layout, const uint8_t *scene, size_t bytes);

/* Run once before motion on every ordinary field update, including ground ticks. */
void bfm_woods_pole_begin_field(BfmWoodsPoleGrip *grip);

/* Once per eligible ordinary airborne update, after proposing the feet. A
 * contact centers X/Z and preserves the proposed Y and incoming yaw. Initialize
 * grip to zero on room entry. On success select mounted body 20, frozen. */
int bfm_woods_pole_catch(const BfmWoodsPoleLayout *layout, BfmWoodsPoleGrip *grip,
                         const int16_t prior[3], int16_t feet[3]);

/* Once per attached field update. ASCEND and DESCEND select mounted body
 * slots 20 and 21 at normal playback. HOLD retains the selection and rate.
 * An active field interaction consumes Cross; normal grip input continues.
 * JUMP asks the normal jump controller to choose its takeoff and heading.
 * FALL resumes free movement at the resulting height with no regrab delay. */
BfmWoodsPoleMove bfm_woods_pole_step(const BfmWoodsPoleLayout *layout, BfmWoodsPoleGrip *grip,
    BfmFieldInput input, int interaction_target_active, int16_t feet[3], unsigned *yaw);

#endif
