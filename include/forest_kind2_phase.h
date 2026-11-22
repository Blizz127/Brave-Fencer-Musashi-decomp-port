#ifndef MUSASHI_FOREST_KIND2_PHASE_H
#define MUSASHI_FOREST_KIND2_PHASE_H
#include "forest_action.h"

extern u32 D_8011F9C4;
s32 func_80029178(u32 index);
s32 func_80149744(ForestPlayerState *actor);
void func_80149788(ForestPlayerState *actor);
s32 func_80148AFC(ForestPlayerState *actor);
s32 func_80146D90(ForestPlayerState *actor);
s32 func_801541D8(ForestPlayerState *actor, s32 index, s32 count);
void func_80147078(ForestPlayerState *actor, u16 kind);
s32 func_8015A264(ForestPlayerState *actor);
s32 func_80159B70(ForestPlayerState *actor);
void func_80161CD0(ForestPlayerState *actor, u32 value);
s32 func_801725A4(ForestPlayerState *actor);
void func_8015B950(ForestPlayerState *actor);

#if defined(__mips__) && !defined(__LP64__)
extern u8 D_8018914C[];
s32 func_80013294(const s16 *first, const s16 *last);
#define FOREST_DISTANCE(first, last) func_80013294((first), (last))
#else
/* Only the first three signed halfwords from each point reach the game math. */
s32 bfm_forest_kind2_distance(const s16 first[3], const s16 last[3]);
#define FOREST_DISTANCE(first, last) bfm_forest_kind2_distance((first), (last))
#endif
#endif
