/* SPDX-License-Identifier: MIT */
#include "forest_foot_target.h"

extern s32 func_8004CFEC(s32 dx, s32 dz);

/* Horizontal bearing from a whole-coordinate field point to the actor. */
s32 func_8014C568(const ForestTargetOwner *actor, const s16 point[3])
{
    s32 dx = (s16)actor->position_halves[1] - point[0];
    s32 dz = (s16)actor->position_halves[5] - point[2];
    return func_8004CFEC(dx, dz) & 0xfff;
}
