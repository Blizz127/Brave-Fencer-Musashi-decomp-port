/* SPDX-License-Identifier: MIT */
#include "valley_rock_fall.h"
#include "field_waypoint.h"
#include "forest_pose.h"

#include <math.h>

static int whole(int32_t value)
{
    return value / 65536 - (value % 65536 < 0);
}

void bfm_valley_rock_fall_begin(BfmValleyRockFall *fall, const int32_t position_q16[3],
                                const int16_t landing[3])
{
    int dx = landing[0] - whole(position_q16[0]);
    int dy = landing[1] - whole(position_q16[1]);
    int dz = landing[2] - whole(position_q16[2]);
    int yaw = (bfm_waypoint_heading(-dz, dx) - 1024) & 4095;

    /* The bearing projects the horizontal delta onto the sampled heading.
     * The acceleration then uses the model rotation's folded quarter wave. */
    double radians = yaw * (6.28318530717958647693 / 4096);
    int sn = (int)lround(4096 * sin(radians)), cs = (int)lround(4096 * cos(radians));
    int projection = dx * sn + dz * cs;
    int forward = -(projection / 4096 - (projection % 4096 < 0));
    int pitch = bfm_waypoint_heading(dy, forward);
    const int16_t origin[3] = {0};
    BfmForestTransform aim;
    bfm_forest_actor_pitch_yaw((unsigned)pitch, (unsigned)yaw, origin, &aim);
    *fall = (BfmValleyRockFall){0};
    /* Local forward is -1.5 in Q16, applied to a Q12 rotation column. */
    for (unsigned i = 0; i < 3; ++i)
        fall->acceleration[i] = -24 * aim.rotation[i][2];
}

int bfm_valley_rock_fall_step(BfmValleyRockFall *fall, const BfmVillageGround *ground,
                              const int16_t probe_offset[3], int32_t position_q16[3])
{
    int16_t from[3], to[3];
    for (unsigned i = 0; i < 3; ++i) {
        from[i] = (int16_t)(whole(position_q16[i]) + probe_offset[i]);
        fall->velocity[i] = (int32_t)((uint32_t)fall->velocity[i] + (uint32_t)fall->acceleration[i]);
        position_q16[i] = (int32_t)((uint32_t)position_q16[i] + (uint32_t)fall->velocity[i]);
        to[i] = (int16_t)(whole(position_q16[i]) + probe_offset[i]);
    }
    uint16_t material;
    unsigned hit = bfm_village_ground_support_probe(ground, 1, from, to, &material);
    if (hit) {
        for (unsigned i = 0; i < 3; ++i) {
            int16_t corrected = (int16_t)(to[i] - probe_offset[i]);
            position_q16[i] = corrected * 65536 + (uint16_t)position_q16[i];
        }
    }
    if (hit & 0x6000u) fall->velocity[1] = 0;
    return (hit & 0x2000u) != 0;
}
