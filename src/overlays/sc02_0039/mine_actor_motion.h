#ifndef BFM_MINE_ACTOR_MOTION_H
#define BFM_MINE_ACTOR_MOTION_H

#include "psx_types.h"

typedef struct {
    u32 type_and_state;
    u32 position[3];
    u32 velocity[3];
    u32 reserved_1c[6];
    u32 vertical_step;
    u32 reserved_38[3];
    u32 flags;
} MineActorMotion;

/* These additions preserve the original 32-bit fixed-point wrap. */
u32 func_801473EC(MineActorMotion *actor);
u32 func_80147460(MineActorMotion *actor);

#endif
