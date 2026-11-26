/* SPDX-License-Identifier: MIT */
#ifndef BFM_FIELD_WAYPOINT_H
#define BFM_FIELD_WAYPOINT_H

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    int16_t yaw;
    int32_t speed_q16;
} BfmWaypointInput;

static inline int bfm_waypoint_heading(int64_t x, int64_t z)
{
    int64_t a = llabs(x), b = llabs(z);
    if (!a && !b) return 0;
    int ratio = (int)(a <= b ? a*1024/b : b*1024/a);
    int angle = (int)lround(atan((ratio+0.375)/1024.0)/0.0015339807878856412);
    if (a > b) angle = 1024-angle;
    if (z < 0) angle = 2048-angle;
    return (x < 0 ? -angle : angle) & 4095;
}

static inline int bfm_waypoint_axis(int angle, int power)
{
    int value = (int)lround(4096*sin((angle & 4095)*0.0015339807878856412))/32;
    if (power) value = value*power/256;
    return value < -127 ? -127 : value > 127 ? 127 : value;
}

/* The input service measures a single axis directly. For diagonals it
 * quantizes the squared length to a base-four mantissa before taking its root. */
static inline int bfm_pad_magnitude(int x, int z)
{
    if (!x || !z) return abs(x) + abs(z);
    unsigned squared = (unsigned)(x*x + z*z), shift = 0;
    while ((squared >> shift) > 255) shift += 2;
    return (int)sqrt((double)((squared >> shift) << shift));
}

/* A scripted waypoint drives the same camera-relative stick as field walking.
 * The player image faces local -Z. Zero power selects full stick deflection. */
static inline BfmWaypointInput bfm_waypoint_input(int dx, int dz, int power, int camera)
{
    int relative = (bfm_waypoint_heading(dx, dz) - camera) & 4088;
    int x = bfm_waypoint_axis(relative, power);
    int z = bfm_waypoint_axis(relative + 1024, power);
    int magnitude = bfm_pad_magnitude(x, z);
    if (magnitude > 127) magnitude = 127;
    return (BfmWaypointInput){
        .yaw = (int16_t)((unsigned)(camera + 2048 + bfm_waypoint_heading(x, z)) & 4095),
        .speed_q16 = 11 * 65536 * magnitude / 127
    };
}
#endif
