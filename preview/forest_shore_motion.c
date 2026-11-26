/* SPDX-License-Identifier: MIT */
#include "forest_shore_motion.h"

#include <limits.h>
#include <math.h>

static int shore_ground(const BfmForestField *field, const int16_t at[3], int z,
                        BfmForestContact *ground)
{
    if (z < INT16_MIN || z > INT16_MAX || at[1] < INT16_MIN + 4 ||
        !bfm_forest_ground(field, at[0], (int16_t)(at[1] - 4), (int16_t)z, ground))
        return 0;
    return !ground->flags && ground->slope == 0x2000 && !ground->normal[0] &&
           ground->normal[1] < 0 &&
           (!ground->normal[2] || ground->normal[1] == 4 * ground->normal[2]);
}

int bfm_forest_shore_begin(BfmForestShoreMotion *motion, const BfmForestField *field,
                           const int16_t placement[3])
{
    BfmForestContact ground;
    if (!motion || !field || !placement || !shore_ground(field, placement, placement[2], &ground) ||
        !ground.normal[2] || ground.height != placement[1] + 1)
        return 0;
    *motion = (BfmForestShoreMotion){
        .position = {placement[0], ground.height, placement[2]},
        .y_q16 = (int32_t)ground.height * 65536
    };
    return 1;
}

/* Walking carries the half-unit vertical fraction through integer foot contacts.
 * On the quarter-grade, the contact also moves the base down the slope normal
 * by one whole horizontal unit. The two-unit leading clearance samples the
 * mounted plane after this adjustment. The flat has no lateral correction. */
static int step_forward(BfmForestShoreMotion *motion, const BfmForestField *field)
{
    int z = motion->position[2] - 4;
    int y = motion->position[1] + 1;
    int fraction = ((uint32_t)motion->y_q16 & 0xffffu) + 32768;
    y += fraction / 65536;
    fraction %= 65536;

    BfmForestContact ground;
    if (!shore_ground(field, motion->position, z, &ground))
        return 0;
    if (ground.normal[2] && y > ground.height) {
        --z;
        if (!shore_ground(field, motion->position, z - 2, &ground))
            return 0;
    }
    if (y > ground.height)
        y = ground.height;
    motion->position[1] = (int16_t)y;
    motion->position[2] = (int16_t)z;
    motion->y_q16 = y * 65536 + fraction;
    return 1;
}

static unsigned bearing(int x, int z)
{
    unsigned a = (unsigned)(x < 0 ? -x : x);
    unsigned b = (unsigned)(z < 0 ? -z : z);
    if (!a && !b)
        return 0;
    unsigned ratio = a <= b ? a * 1024 / b : b * 1024 / a;
    const double radians = 6.2831853071795864769 / 4096;
    int angle = (int)lround(atan((ratio + 0.375) / 1024.0) / radians);
    if (a > b)
        angle = 1024 - angle;
    if (z < 0)
        angle = 2048 - angle;
    return (unsigned)((x < 0 ? -angle : angle) & 4095);
}

/* Limit the first notice to the witnessed frontal encounter. Both planar
 * and spatial separation must be inside the measured positive bound.
 * A single approach does not distinguish the game's exact threshold. */
static int within_notice(const int16_t at[3], const int16_t player[3])
{
    int64_t dx = at[0] - player[0], dy = at[1] - player[1], dz = at[2] - player[2];
    int64_t horizontal = dx * dx + dz * dz;
    return dz >= 0 && horizontal <= 38 * 38 + 232 * 232 &&
           horizontal + dy * dy <= 38 * 38 + 232 * 232 + 67 * 67;
}

int bfm_forest_shore_tick(BfmForestShoreMotion *motion, const BfmForestField *field,
                          const int16_t player[3], unsigned selected_frame_count)
{
    if (!motion || !field || !player || motion->frame >= selected_frame_count)
        return 0;
    if (motion->phase == BFM_SHORE_WALK) {
        if (!step_forward(motion, field))
            return 0;
        if (within_notice(motion->position, player)) {
            motion->phase = BFM_SHORE_NOTICE;
            motion->frame = 0;
        } else {
            motion->frame = (motion->frame + 1) % selected_frame_count;
        }
        return 1;
    }
    if (motion->phase != BFM_SHORE_NOTICE)
        return 0;
    int dx = motion->position[0] - player[0], dz = motion->position[2] - player[2];
    int delta = (int)bearing(dx, dz) - (int)motion->yaw;
    if (delta > 2048)
        delta -= 4096;
    else if (delta < -2048)
        delta += 4096;
    motion->yaw = (uint16_t)((motion->yaw + delta / 4) & 4095);
    motion->frame = (motion->frame + 1) % selected_frame_count;
    return 1;
}
