#ifndef BFM_MINE_ACTOR_STEP_H
#define BFM_MINE_ACTOR_STEP_H

#include "mine_terrain_vector.h"

typedef union {
    u32 value;
    struct { u16 fraction, integer; } half;
} MineStepFixed;

typedef struct {
    u16 type, phase;
    MineStepFixed position[3];
    u32 velocity[3];
    u8 before_previous[0x38 - 0x1C];
    MineStepFixed previous[3];
    u32 acceleration[3];
} MineStepActor;

s32 func_8012CC88(MineStepActor *actor, s32 mode, SceneProbeVector *offset);

#endif
