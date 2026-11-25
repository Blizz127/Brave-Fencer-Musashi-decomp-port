#ifndef MUSASHI_FOREST_PLAYER_LOCOMOTION_H
#define MUSASHI_FOREST_PLAYER_LOCOMOTION_H
#include "forest_actor_mask.h"

/* The published controller type is the byte between the input timer and held buttons. */
#define FOREST_MOVEMENT_CONTROLLER(actor) (((const u8 *)(actor))[0xa9])

typedef struct { u8 pad00[0x26]; s16 stat26; u8 pad28[6]; s16 stat2e; } ForestMode1Stats;
extern ForestMode1Stats D_80078E78;
extern s16 D_800B9A08;
extern u16 D_800B99DA;
#if defined(__mips__) && !defined(__LP64__)
extern u8 D_800D5428[];
extern s32 D_8018913C[];
extern u32 D_801891B8[];
#define ANIMATION_ADDRESS ((u32)D_800D5428)
#define MOVEMENT_VECTOR(actor) func_80146DB8((s32 *)(actor), D_8018913C)
#else
#define ANIMATION_ADDRESS 0x800d5428u
s32 bfm_forest_action_vector(ForestPlayerState *,u32);
#define MOVEMENT_VECTOR(actor) bfm_forest_action_vector((actor), 0x8018913cu)
#endif
#define DISPATCH_KIND(actor) FOREST_PLAYER_CALL(801891B8, (actor)->kind, (actor))
s32 func_80149D9C(ForestPlayerState *);
void func_80149FA8(ForestPlayerState *);
s32 func_80149FB0(ForestPlayerState *);
s32 func_80161B84(ForestPlayerState *);
s32 func_80161B18(ForestPlayerState *);
s32 func_8014A048(ForestPlayerState *);
s32 func_8014A218(ForestPlayerState *);
s32 func_80161BE0(ForestPlayerState *);
s32 func_801619A4(ForestPlayerState *);
s32 func_80149AA8(ForestPlayerState *);
s32 func_80149AD4(ForestPlayerState *);
s32 func_80149B54(ForestPlayerState *);
s32 func_80149BAC(ForestPlayerState *);
s32 func_801498E0(u32);
s32 func_80149954(ForestPlayerState *);
s32 func_80149A64(ForestPlayerState *);
s32 func_800CCCC0(ForestPlayerState *);
s32 func_80149CD4(ForestPlayerState *);
s32 func_8015E880(ForestPlayerState *);
s32 func_801496D4(ForestPlayerState *);
s32 func_80149704(ForestPlayerState *);
s32 func_801488A8(ForestPlayerState *);
s32 func_8014C010(s32, s32);
void func_80147078(ForestPlayerState *, u16);
s32 func_80159B70(ForestPlayerState *);
s32 func_8015ABD4(s32, s32, s32);
s32 func_8015AC48(ForestPlayerState *);
s32 func_80148AFC(ForestPlayerState *);
s32 func_80154150(ForestPlayerState *, s32);
s32 func_80154A74(ForestPlayerState *, s32);
s32 func_80146DB8(s32 *, s32 *);
void func_80147AD4(ForestPlayerState *, s32, s32, s32);
u16 func_801487F4(ForestPlayerState *);
u32 func_8016F1C4(void);
u8 func_80148824(ForestPlayerState *);
void func_80147A84(ForestPlayerState *);
s32 func_8015AB7C(ForestPlayerState *);
s32 func_801541D8(ForestPlayerState *, s32, s32);
s32 func_801473EC(ForestPlayerState *);
s32 func_8014D738(ForestPlayerState *);
s32 func_8015C6E0(ForestPlayerState *);
s32 func_8014E934(ForestPlayerState *);
void func_80155440(ForestPlayerState *);
void func_801553C0(ForestPlayerState *);
s32 func_80161278(ForestPlayerState *);
s32 func_80161C24(ForestPlayerState *, u32);
s32 func_801483E8(ForestPlayerState *);
s32 func_8015A3C8(ForestPlayerState *);
#endif
