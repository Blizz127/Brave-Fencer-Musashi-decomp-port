#ifndef MUSASHI_FOREST_TRANSITION_H
#define MUSASHI_FOREST_TRANSITION_H
#include "forest_action.h"
#include "forest_floor.h"

/* B4 is the Z halfword in the normal stored at B0. */
#if defined(__mips__) && !defined(__LP64__)
extern s16 D_801152B4;
#define FOREST_NORMAL_Z D_801152B4
#else
#define FOREST_NORMAL_Z ((s16)D_801152B0.z)
#endif
void func_80147078(ForestActionPlayer *actor, u16 kind);
void func_801553A8(ForestActionPlayer *actor);
s32 func_80146CA0(ForestActionPlayer *actor);
s32 func_8015BE94(ForestActionPlayer *actor);
s32 func_8015BDD0(ForestActionPlayer *actor);
s32 func_8015BF48(ForestActionPlayer *actor);
s32 func_8015C0C4(ForestActionPlayer *actor);
#endif
