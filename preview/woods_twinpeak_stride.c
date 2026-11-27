/* SPDX-License-Identifier: MIT */
#include "woods_twinpeak_stride.h"
#include "forest_pose.h"
#include <limits.h>
#include <string.h>

/* The turned model and the walk stick both use local -Z as forward. */
int bfm_twinpeak_turn(BfmBedroomRoot *r, unsigned goal)
{
    int delta = (int)(goal & 4095) - (int)r->yaw;
    if (delta > 2048) delta -= 4096;
    if (delta < -2048) delta += 4096;
    int step = delta/4;
    if (!step) step = delta/2;
    if (!step) step = delta;
    r->yaw = (unsigned)((int)r->yaw+step) & 4095;
    return !delta;
}

int bfm_twinpeak_footing(const BfmVillageGround *ground, BfmBedroomRoot *r, int16_t to[3])
{
    BfmForestContact contact;
    if (bfm_village_ground_step(ground,r->xyz,to,&contact)
        != BFM_VILLAGE_GROUND_FOOTING) return 0;
    memcpy(r->xyz,to,sizeof r->xyz);
    memcpy(r->normal,contact.normal,sizeof r->normal);
    return 1;
}

int bfm_twinpeak_stride_target(const BfmBedroomRoot *r, int32_t position[3],
                              int32_t speed, unsigned yaw, int16_t to[3])
{
    BfmForestTransform rotation;
    memcpy(to,r->xyz,sizeof r->xyz);
    bfm_forest_actor_root(yaw,r->xyz,&rotation);
    for (unsigned i = 0; i < 2; ++i) {
        unsigned axis = 2*i;
        int64_t v = (int64_t)speed * -rotation.rotation[axis][2];
        int64_t q = position[axis] + v/4096 - (v%4096 < 0);
        int64_t whole = q/65536 - (q%65536 < 0);
        if (whole < INT16_MIN || whole > INT16_MAX) return 0;
        to[axis] = (int16_t)whole;
        position[axis] = (int32_t)q;
    }
    return 1;
}

int bfm_twinpeak_stride(const BfmVillageGround *ground, BfmBedroomRoot *r, int32_t position[3],
                        int32_t speed, unsigned yaw)
{
    int16_t to[3];
    return bfm_twinpeak_stride_target(r, position, speed, yaw, to) &&
           bfm_twinpeak_footing(ground, r, to);
}
