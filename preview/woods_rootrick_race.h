/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_ROOTRICK_RACE_H
#define BFM_WOODS_ROOTRICK_RACE_H

#include "bedroom_motion.h"
#include "field_input.h"

typedef struct {
    int expect_square;
    int queued;
    int stroke_y;
} BfmRootrickGrip;

/* Enter the ordinary player action after attaching to a climbable wall.
 * Re-enter when the player attaches above the intermediate ledge. */
int bfm_rootrick_grip_begin(BfmRootrickGrip *grip, BfmBedroomMotion *motion);

/* One active field while attached. This advances the borrowed motion once.
 * Pass the resulting displacement through the live field's wall/ledge tests.
 * The pose carries ordinary face and weapon data, including hand contacts. */
int bfm_rootrick_grip_step(BfmRootrickGrip *grip, BfmBedroomMotion *motion,
                           BfmFieldInput input, BfmBedroomPose *pose, int *delta_y);

/* The race checks Musashi's ordinary action after he and the rival have been
 * placed at the wall. An airborne character above the rim has yet to finish. */
typedef enum {
    BFM_ROOTRICK_UNDECIDED, BFM_ROOTRICK_LOSS, BFM_ROOTRICK_WIN, BFM_ROOTRICK_CAPE_RUSE
} BfmRootrickRaceOutcome;

typedef enum {
    BFM_ROOTRICK_RACING, BFM_ROOTRICK_STUMBLING, BFM_ROOTRICK_PULLING_UP, BFM_ROOTRICK_AT_TOP
} BfmRootrickRivalStatus;

typedef struct {
    int control_claimed;
    int in_grace;
    unsigned remaining;
    BfmRootrickRaceOutcome result;
} BfmRootrickFinish;

/* Relative height controls the rival's normal ascending speed, in 16.16.
 * Apply this before testing the real crest and mounted obstacle actors. */
int32_t bfm_rootrick_rival_speed(int32_t hero_y_q16, int32_t rival_y_q16);
int bfm_rootrick_rival_at_crest(int rival_y);

/* One field, before advancing the rival. Grounded and attached are distinct
 * player action flags. A change in control_claimed yields the summit cut to
 * the scene while an interrupted rival is still recovering. */
BfmRootrickRaceOutcome bfm_rootrick_finish_step(BfmRootrickFinish *finish,
    BfmRootrickRivalStatus rival, int hero_y, int grounded, int attached);

#endif
