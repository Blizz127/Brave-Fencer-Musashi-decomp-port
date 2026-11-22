#ifndef BFM_MINE_TERRAIN_ACTION_H
#define BFM_MINE_TERRAIN_ACTION_H

#include "../main_0012/scene_object_probes.h"

typedef struct {
    u16 x, y, z;
    union {
        u16 flags;
        u8 action;
    } surface;
} MineActionPoint;

typedef struct MineTerrainActor {
    u8 state[4];
    u16 position[6];
    u8 before_flags[0x44 - 0x10];
    u32 flags;
    u8 before_previous[0x88 - 0x48];
    u16 previous[3];
    u8 before_action[0x16c - 0x8e];
    u16 action;
    u16 surface_class;
} MineTerrainActor;

extern SceneProbeVector D_801152B0;
extern s32 D_801152BC;
s32 func_80133784(s16 mode, MineActionPoint *from, MineActionPoint *to);
s32 func_80029178(u32 flag);
s32 func_8014FA70(MineTerrainActor *actor);

#endif
