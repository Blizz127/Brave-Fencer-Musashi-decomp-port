#ifndef BFM_MINE_CHILD_SPAWN_H
#define BFM_MINE_CHILD_SPAWN_H

#include "mine_actor_spawn.h"

typedef struct MineEmitterActor {
    u8 before_timer[0x1c];
    s32 timer;
    u8 before_subtype[0x70 - 0x20];
    s16 subtype;
    u8 before_distance[0xe0 - 0x72];
    s32 distance;
    u8 before_index[0xfe - 0xe4];
    s16 index;
    s16 range;
} MineEmitterActor;

typedef struct {
    u8 subtype, delay;
} MineEmitterStep;

extern MineEmitterStep D_8019B594[];
void func_8017E794(MineEmitterActor *actor);
s32 func_8012BEE8(MineEmitterActor *actor);
s32 func_8005C49C(void);
void func_8017E640(MineEmitterActor *actor);

#endif
