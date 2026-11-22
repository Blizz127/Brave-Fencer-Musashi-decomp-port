#ifndef MUSASHI_FOREST_ACTION_H
#define MUSASHI_FOREST_ACTION_H
#include "forest_player_state.h"

typedef ForestPlayerState ForestActionPlayer;

#if defined(__mips__) && !defined(__LP64__)
extern u8 D_800D5F44[], D_800D5F34[], D_800D5F14[], D_800D5F04[];
extern u8 D_8018911C[], D_800D49F4[];
extern u32 D_801891B8[];
s32 func_8005C49C(void);
s32 func_80146DB8(s32 *actor_words, s32 *vector);
#define ACTION_DATA(address) ((u32) D_##address)
#define ACTION_SAMPLE() func_8005C49C()
#define ACTION_VECTOR(actor, address) func_80146DB8((s32 *)(actor), (s32 *)(address))
#else
/* Resolve the three vector words in owned game memory and preserve the
 * original sample service's state. An unavailable service stops this query. */
s32 bfm_forest_action_sample(void);
s32 bfm_forest_action_vector(ForestActionPlayer *actor, u32 address);
#define ACTION_DATA(address) 0x##address##u
#define ACTION_SAMPLE() bfm_forest_action_sample()
#define ACTION_VECTOR(actor, address) bfm_forest_action_vector((actor), (address))
#endif

extern u16 D_800B99DA;
void func_80159C84(ForestActionPlayer *actor);
void func_800CCCC0(ForestActionPlayer *);
s32 func_80147324(u16);
s32 func_801473EC(ForestActionPlayer *);
void func_80147A84(ForestActionPlayer *);
s32 func_801483E8(ForestActionPlayer *);
s32 func_801488A8(ForestActionPlayer *);
s32 func_801496D4(ForestActionPlayer *);
s32 func_80149704(ForestActionPlayer *);
s32 func_801498E0(u32 actor_address);
void func_80149954(ForestActionPlayer *);
s32 func_80149A64(ForestActionPlayer *);
s32 func_80149AA8(ForestActionPlayer *);
void func_80149AD4(ForestActionPlayer *);
s32 func_80149B54(ForestActionPlayer *);
void func_80149BAC(ForestActionPlayer *);
s32 func_80149CD4(ForestActionPlayer *);
s32 func_80149D9C(ForestActionPlayer *);
void func_80149FA8(ForestActionPlayer *);
s32 func_80149FB0(ForestActionPlayer *);
s32 func_8014A738(ForestActionPlayer *);
void func_8014A830(ForestActionPlayer *);
s32 func_8014C010(s32 actor_address, s32 request);
s32 func_80154150(ForestActionPlayer *, s32);
s32 func_80154190(ForestActionPlayer *, s32);
s32 func_80154A74(ForestActionPlayer *, s32);
s32 func_80154B4C(ForestActionPlayer *, u32);
void func_80155440(ForestActionPlayer *);
s32 func_8015A230(ForestActionPlayer *);
void func_8015E880(ForestActionPlayer *);
s32 func_80161208(ForestActionPlayer *);
s32 func_801619A4(ForestActionPlayer *);
s32 func_80161B18(ForestActionPlayer *);
s32 func_80161B84(ForestActionPlayer *);
s32 func_80161BE0(ForestActionPlayer *);
u32 func_8016F1AC(void);
s32 func_80172590(ForestActionPlayer *);
#endif
