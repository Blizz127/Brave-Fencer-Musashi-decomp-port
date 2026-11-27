/* SPDX-License-Identifier: MIT */
#include "forest_shore_behavior.h"
#include "forest_shore_motion.h"

enum { CLOSED = 1, OPENING, WATCHING, TURNING, STRIKING };

void bfm_forest_shore_behavior_begin(BfmForestShoreBehavior *behavior)
{
    *behavior = (BfmForestShoreBehavior){ .state = CLOSED };
}

void bfm_forest_shore_behavior_ready(BfmForestShoreBehavior *behavior)
{
    behavior->state = WATCHING;
    behavior->tick = 0;
    behavior->timer = 45;
    behavior->outside_ticks = 0;
}

uint32_t bfm_forest_shore_behavior_clip(const BfmForestShoreBehavior *behavior)
{
    switch (behavior->state) {
    case CLOSED: return UINT32_C(0x8019731c);
    case OPENING: return UINT32_C(0x801973c4);
    case WATCHING: return UINT32_C(0x80196f3c);
    case TURNING:
    case STRIKING: return UINT32_C(0x801970dc);
    default: return 0;
    }
}

static int64_t distance_squared(const int16_t at[3], const int16_t target[3])
{
    int64_t total = 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        int64_t delta = at[axis] - target[axis];
        total += delta * delta;
    }
    return total;
}

static void turn(BfmForestShoreBehavior *behavior, const int16_t at[3],
                 const int16_t player[3])
{
    /* The model accumulates signed yaw. Wrapping the *difference* selects
     * the short arc while preserving the signed animation orientation. */
    int x = at[0] - player[0], z = at[2] - player[2];
    int target = (x || z) ? (int)bfm_forest_shore_bearing(x, z) : 3072;
    int difference = target - (behavior->yaw & 4095);
    if (difference > 2048) difference -= 4096;
    if (difference < -2048) difference += 4096;
    int step = difference / 4;
    if (!step && difference) step = difference > 0 ? 1 : -1;
    behavior->yaw = (int16_t)(behavior->yaw + step);
}

unsigned bfm_forest_shore_behavior_tick(BfmForestShoreBehavior *behavior,
                                        const int16_t at[3], const int16_t player[3],
                                        const int16_t target[3], int targetable)
{
    int dz = at[2] - player[2];
    unsigned retirement = (dz <= -864 || dz >= 864) ? BFM_SHORE_FIGURE_RETIRE : 0;
    switch (behavior->state) {
    case CLOSED:
        behavior->tick = (behavior->tick + 1) % 20;
        if (targetable && distance_squared(at, target) <= 384 * 384) {
            behavior->state = OPENING;
            behavior->tick = 0;
        }
        return retirement;
    case OPENING:
        if (++behavior->tick == 20) bfm_forest_shore_behavior_ready(behavior);
        return 0;
    case WATCHING: {
        behavior->tick = (behavior->tick + 1) % 20;
        if (behavior->timer && --behavior->timer) return retirement;
        int64_t distance = targetable ? distance_squared(at, target) : INT64_MAX;
        if (distance <= 192 * 192) {
            behavior->state = TURNING;
            behavior->tick = 0;
            if (!behavior->noticed) {
                behavior->noticed = 1;
                return BFM_SHORE_FIGURE_NOTICE;
            }
            return 0;
        }
        ++behavior->outside_ticks;
        if (behavior->outside_ticks >= 121 && behavior->outside_ticks <= INT16_MAX &&
            distance > 384 * 384) {
            behavior->state = CLOSED;
            behavior->tick = 0;
            return 0;
        }
        return retirement;
    }
    case TURNING:
        /* Even if the player temporarily cannot be targeted, its cached
         * position continues to define the attack's heading. */
        turn(behavior, at, target);
        if (++behavior->tick == 30) behavior->state = STRIKING;
        return 0;
    case STRIKING:
        if (++behavior->tick == 50) bfm_forest_shore_behavior_ready(behavior);
        return behavior->tick == 42 ? BFM_SHORE_FIGURE_ATTACK_CONTACT : 0;
    default:
        return 0;
    }
}
