/* SPDX-License-Identifier: MIT */
#include "woods_rootrick_race.h"

int bfm_rootrick_grip_begin(BfmRootrickGrip *grip, BfmBedroomMotion *motion)
{
    if (!grip || !bfm_bedroom_motion_direct_slot(motion, 24)) return 0;
    *grip = (BfmRootrickGrip){0};
    return 1;
}

int bfm_rootrick_grip_step(BfmRootrickGrip *grip, BfmBedroomMotion *motion,
                           BfmFieldInput input, BfmBedroomPose *pose, int *delta_y)
{
    if (!grip || !pose || !delta_y ||
        !bfm_bedroom_motion_step(motion, BFM_BEDROOM_NONE, pose)) return 0;

    if (!grip->queued && (grip->expect_square ? input.square_pressed : input.triangle_pressed)) {
        grip->queued = 1;
        grip->expect_square = !grip->expect_square;
    }

    int held = bfm_bedroom_motion_marker(motion) == BFM_BODY_HOLD;
    if (held || bfm_bedroom_motion_looped(motion)) {
        /* Wrapping presents the initial pose. Its explicit hold follows on
         * the next field, so both boundaries can consume a buffered edge. */
        if (grip->queued) {
            if (held) bfm_bedroom_motion_resume(motion);
            grip->stroke_y = input.y >= 65 ? 4 : -4;
            grip->queued = 0;
        }
        *delta_y = 0;
    } else {
        *delta_y = grip->stroke_y;
    }
    return 1;
}

int32_t bfm_rootrick_rival_speed(int32_t hero_y_q16, int32_t rival_y_q16)
{
    const int unit = 65536;
    int64_t distance = (int64_t)hero_y_q16 - rival_y_q16 + 144 * unit;
    int scale = (int)(distance / (16 * unit));
    if (scale < -8) scale = -8;
    if (scale > 0) scale = 0;
    return -2 * unit + scale * (unit / 2);
}

int bfm_rootrick_rival_at_crest(int rival_y)
{
    return rival_y <= -2386;
}

BfmRootrickRaceOutcome bfm_rootrick_finish_step(BfmRootrickFinish *finish,
    BfmRootrickRivalStatus rival, int hero_y, int grounded, int attached)
{
    if (finish->result) return finish->result;
    int at_top = grounded && hero_y <= -2562;
    if (finish->in_grace) {
        if (finish->remaining) --finish->remaining;
        if (!finish->remaining && (grounded || attached))
            finish->result = at_top ? BFM_ROOTRICK_CAPE_RUSE : BFM_ROOTRICK_LOSS;
    } else if (at_top) {
        finish->control_claimed = 1;
        if (rival == BFM_ROOTRICK_AT_TOP) finish->result = BFM_ROOTRICK_CAPE_RUSE;
        else if (rival == BFM_ROOTRICK_RACING) finish->result = BFM_ROOTRICK_WIN;
    } else if ((grounded || attached) && rival == BFM_ROOTRICK_AT_TOP) {
        finish->control_claimed = 1;
        finish->in_grace = 1;
        finish->remaining = 16;
    }
    return finish->result;
}
