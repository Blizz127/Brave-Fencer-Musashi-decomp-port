#ifndef MUSASHI_FOREST_FOOT_TARGET_H
#define MUSASHI_FOREST_FOOT_TARGET_H

#include "psx_types.h"

typedef struct {
    /* The signed world coordinates are transferred as three halfwords. */
    u16 position[3];
    s16 status;
} ForestTarget;

typedef struct {
    u32 kind_and_action;
    /* Low and high halfwords of each player Q16 coordinate in XYZ order. */
    u16 position_halves[6];
    u8 before_foot[0x154 - 0x10];
    ForestTarget supplied_foot;
    ForestTarget target;
} ForestTargetOwner;

void func_8014C860(ForestTargetOwner *actor, const ForestTarget *foot);
void func_8014C88C(ForestTargetOwner *actor);
void func_8014C8C8(ForestTargetOwner *actor, const ForestTarget *foot);

#endif
