/* SPDX-License-Identifier: MIT */
#include "forest_shore_motion.h"

#include <limits.h>
#include <math.h>
#include <stdlib.h>

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
        ground.height != placement[1] + (ground.normal[2] != 0))
        return 0;
    *motion = (BfmForestShoreMotion){
        .position = {placement[0], ground.height, placement[2]},
        .y_q16 = (int32_t)ground.height * 65536
    };
    return 1;
}

/* Ground resolution adjusts the integer position, preserving each low half. */
static int step_forward(BfmForestShoreMotion *motion, const BfmForestField *field, int speed)
{
    BfmForestTransform facing;
    bfm_forest_actor_root(motion->travel_yaw, motion->position, &facing);
    /* The model faces local -Z. Keep fractions as the stride turns. */
    int32_t dx = motion->x_fraction - speed * 16 * facing.rotation[0][2];
    int32_t dz = motion->z_fraction - speed * 16 * facing.rotation[2][2];
    int x = motion->position[0] + (dx - (uint16_t)dx) / 65536;
    int z = motion->position[2] + (dz - (uint16_t)dz) / 65536;
    int fraction = (uint16_t)motion->y_q16 + 32768;
    int y = motion->position[1] + 1 + fraction / 65536;
    if (x < INT16_MIN || x > INT16_MAX || y > INT16_MAX || z < INT16_MIN || z > INT16_MAX)
        return 0;
    int16_t proposed[3] = {(int16_t)x, (int16_t)y, (int16_t)z};
    BfmForestShoreFloor floor = motion->phase == BFM_SHORE_PURSUIT
        ? bfm_forest_pursuit_floor_step(field, motion->position, proposed)
        : bfm_forest_shore_floor_step(field, motion->position, proposed);
    if (floor == BFM_SHORE_FLOOR_UNKNOWN) return 0;
    if (floor == BFM_SHORE_FLOOR_STEEP) {
        motion->phase = BFM_SHORE_TURN;
        motion->turn_target = (motion->yaw + 2048u) & 4095u;
        motion->patrol_timer = 22;
        motion->frame = 0;
        return 1;
    }
    for (unsigned i = 0; i < 3; ++i)
        motion->position[i] = proposed[i];
    motion->y_q16 = (int32_t)proposed[1] * 65536 + (uint16_t)fraction;
    motion->x_fraction = (uint16_t)dx;
    motion->z_fraction = (uint16_t)dz;
    return 1;
}

unsigned bfm_forest_shore_bearing(int x, int z)
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

int bfm_forest_shore_recoil_height(BfmForestShoreMotion *motion, const BfmForestField *field)
{
    int32_t vy = motion->recoil_vy_q16 + 98304;
    int64_t y = (int64_t)motion->y_q16 + vy;
    if (y < INT32_MIN || y > INT32_MAX) return 0;
    uint16_t fraction = (uint16_t)y;
    int16_t endpoint[3] = {
        motion->position[0], (int16_t)((y - fraction) / 65536), motion->position[2]
    };
    int landed;
    if (!bfm_forest_thrown_probe(field, motion->position, endpoint, 0, &landed)) return 0;
    for (unsigned axis = 0; axis < 3; ++axis) motion->position[axis] = endpoint[axis];
    motion->y_q16 = (int32_t)endpoint[1] * 65536 + fraction;
    motion->recoil_vy_q16 = landed ? 0 : vy;
    return 1;
}

/* A patrolling guard compares its world position with the player's
 * target point using a three-dimensional 256-unit notice radius. */
static int within_range(const int16_t at[3], const int16_t player[3], int radius)
{
    int64_t dx = (int)at[0] - player[0], dy = (int)at[1] - player[1];
    int64_t dz = (int)at[2] - player[2];
    return dx * dx + dy * dy + dz * dz <= radius * radius;
}

static int yaw_difference(unsigned target, unsigned yaw)
{
    int delta = (int)target - (int)yaw;
    if (delta > 2048) delta -= 4096;
    if (delta < -2048) delta += 4096;
    return delta;
}

static unsigned turn_step(unsigned heading, unsigned target, int divisor)
{
    int delta = yaw_difference(target, heading);
    int step = delta / divisor;
    if (!step && delta) step = delta > 0 ? 1 : -1;
    return (heading + step) & 4095;
}

static void begin_pursuit(BfmForestShoreMotion *motion)
{
    motion->phase = BFM_SHORE_PURSUIT;
    motion->patrol_timer = 30;
    motion->frame = 0;
}

static void resume_patrol(BfmForestShoreMotion *motion)
{
    motion->phase = BFM_SHORE_WALK;
    motion->frame = 0;
    motion->notice_delay = 30;
}

int bfm_forest_shore_tick(BfmForestShoreMotion *motion, const BfmForestField *field,
                          const int16_t player[3], unsigned selected_frame_count, BfmGameplayRun *run)
{
    if (!motion || !field || !player || motion->frame >= selected_frame_count)
        return 0;
    if (motion->phase == BFM_SHORE_WALK) {
        BfmForestShoreMotion next = *motion;
        if (++next.walk_updates <= 8) {
            /* Facing changes before the guard redirects its stride. */
            next.yaw = (uint16_t)turn_step(next.yaw, next.turn_target, 6);
        } else {
            next.travel_yaw = next.yaw;
        }
        if (!step_forward(&next, field, 4))
            return 0;
        *motion = next;
        if (motion->phase == BFM_SHORE_TURN) return 1;
        if (motion->walk_updates == 60) {
            motion->walk_updates = 0;
            if (run) {
                int offset = (bfm_gameplay_random(run) & 0x800) ? 1024 : -1024;
                motion->turn_target = (uint16_t)((motion->yaw + offset) & 4095);
            }
        }
        if (motion->notice_delay) --motion->notice_delay;
        if (!motion->notice_delay && within_range(motion->position, player, 256)) {
            motion->travel_speed = 4;
            if (motion->noticed_player) {
                /* Reacquisition keeps the patrol counter accumulated since
                 * the last turn. First notice starts its own pursuit clock. */
                motion->pursuit_age = motion->walk_updates;
                begin_pursuit(motion);
            } else {
                motion->noticed_player = 1;
                motion->pursuit_age = 0;
                motion->phase = BFM_SHORE_NOTICE;
                motion->frame = 0;
            }
        } else {
            motion->frame = (motion->frame + 1) % selected_frame_count;
        }
        return 1;
    }
    if (motion->phase == BFM_SHORE_RECOIL) {
        if (!bfm_forest_shore_recoil_height(motion, field))
            return 0;
        /* The impact update consumes the first timer tick. The last one
         * selects the walking clip before its first pose is drawn. */
        if (++motion->frame + 1 == selected_frame_count) {
            motion->phase = BFM_SHORE_WALK;
            motion->frame = 0;
            motion->notice_delay = 30;
        }
        return 1;
    }
    if (motion->phase == BFM_SHORE_PURSUIT) {
        /* Notice pauses movement without clearing the last patrol velocity.
         * The first pursuit step integrates that stride before accelerating. */
        if (!step_forward(motion, field, (int)motion->travel_speed)) return 0;
        if (motion->phase != BFM_SHORE_PURSUIT) return 1;
        int dx = motion->position[0] - player[0], dz = motion->position[2] - player[2];
        unsigned bearing = bfm_forest_shore_bearing(dx, dz);
        /* Velocity is transformed by the last submitted body matrix. The
         * facing change is first visible to the following velocity update. */
        motion->travel_yaw = motion->yaw;
        motion->travel_speed = 8;
        if (motion->patrol_timer >= 22)
            motion->yaw = (uint16_t)turn_step(motion->yaw, bearing, 6);
        ++motion->pursuit_age;
        if (within_range(motion->position, player, 64) &&
            abs(yaw_difference(bearing, motion->yaw)) <= 1024) {
            motion->phase = BFM_SHORE_STRIKE;
            motion->patrol_timer = 20;
            motion->frame = 0;
        } else if (motion->pursuit_age >= 90) {
            resume_patrol(motion);
        } else if (!--motion->patrol_timer) {
            motion->patrol_timer = 30;
            motion->frame = 0;
        } else {
            motion->frame = (motion->frame + 1) % selected_frame_count;
        }
        return 1;
    }
    if (motion->phase == BFM_SHORE_STRIKE || motion->phase == BFM_SHORE_PAUSE ||
        motion->phase == BFM_SHORE_FOLLOW_THROUGH) {
        if (--motion->patrol_timer) {
            motion->frame = (motion->frame + 1) % selected_frame_count;
        } else if (motion->phase == BFM_SHORE_STRIKE &&
                   within_range(motion->position, player, 128)) {
            motion->phase = BFM_SHORE_PAUSE;
            motion->patrol_timer = 10;
            motion->frame = 0;
        } else if (motion->phase == BFM_SHORE_PAUSE) {
            motion->phase = BFM_SHORE_FOLLOW_THROUGH;
            motion->patrol_timer = 24;
            motion->frame = 0;
        } else {
            /* After the close attack the soldier follows the player again. */
            begin_pursuit(motion);
        }
        return 1;
    }
    unsigned target = motion->turn_target;
    if (motion->phase == BFM_SHORE_NOTICE) {
        int dx = motion->position[0] - player[0], dz = motion->position[2] - player[2];
        target = bfm_forest_shore_bearing(dx, dz);
    } else if (motion->phase != BFM_SHORE_TURN) {
        return 0;
    }
    motion->yaw = (uint16_t)turn_step(motion->yaw, target, 4);
    motion->frame = (motion->frame + 1) % selected_frame_count;
    if (motion->phase == BFM_SHORE_NOTICE && motion->frame == 0) {
        begin_pursuit(motion);
    }
    if (motion->phase == BFM_SHORE_TURN && --motion->patrol_timer == 0) {
        motion->phase = BFM_SHORE_WALK;
        motion->notice_delay = 30;
        motion->frame = 0;
    }
    return 1;
}
