#ifndef MUSASHI_FOREST_PLAYER_STATE_H
#define MUSASHI_FOREST_PLAYER_STATE_H

#include "psx_types.h"

typedef struct {
    u16 kind, action;
    u8 pad04[0x40];
    u32 flags;
    u8 pad48[4];
    u8 state, phase;
    u8 pad4e[0xa8 - 0x4e];
    u8 field_a8;
    u8 pada9[0xb4 - 0xa9];
    u32 animation;
    u16 animation_flags;
    u8 padba[0xd0 - 0xba];
    u32 sequence;
    u16 sequence_flags;
    u8 padd6[0xdd - 0xd6];
    u8 mode;
    u8 padde[0x234 - 0xde];
    u32 step, counter;
} ForestPlayerState;

extern ForestPlayerState D_80126B58;

#if defined(__mips__) && !defined(__LP64__)
extern u8 D_80126BA4;
extern u32 D_8018860C[], D_801891AC[], D_8018925C[], D_80189310[];
#define FOREST_CURRENT_STATE D_80126BA4
#define FOREST_ACTION_RESULT(table, index, actor) (((s32 (*)(ForestPlayerState *))D_##table[index])(actor))
#define FOREST_ACTOR_GAME_ADDRESS(actor) ((u32)(actor))
#define FOREST_PLAYER_CALL(table, index, actor) (((void (*)(ForestPlayerState *))D_##table[index])(actor))
#else
#define FOREST_CURRENT_STATE D_80126B58.state
/* Return the original nonzero actor handle, or stop the query if it is unmapped. */
u32 bfm_forest_action_actor_address(ForestPlayerState *actor);
#define FOREST_ACTOR_GAME_ADDRESS(actor) bfm_forest_action_actor_address(actor)
/* Table entries are four-byte game addresses, including on a 64-bit host. */
u32 bfm_forest_player_table_word(u32 table_address, u32 index);
void bfm_forest_player_call(u32 game_address, ForestPlayerState *actor);
/* Some action callers forward the selected callback's returned register. */
s32 bfm_forest_player_call_result(u32 game_address, ForestPlayerState *actor);
#define FOREST_ACTION_RESULT(table, index, actor) \
    bfm_forest_player_call_result(bfm_forest_player_table_word(0x##table##u, (index)), (actor))
/* The owner stops or rolls back the whole query if either lookup is unavailable. */
#define FOREST_PLAYER_CALL(table, index, actor) \
    bfm_forest_player_call(bfm_forest_player_table_word(0x##table##u, (index)), (actor))
#endif

void func_80150B28(ForestPlayerState *actor);
void func_80158638(ForestPlayerState *actor);
void func_8015126C(ForestPlayerState *actor);
void func_8014900C(ForestPlayerState *actor);
void func_80146360(void);
void func_80159968(ForestPlayerState *actor);
void func_80159A20(ForestPlayerState *actor);
s32 func_8015AD3C(ForestPlayerState *actor);
s32 func_8015AD78(ForestPlayerState *actor);
s32 func_8015ADB0(ForestPlayerState *actor);
s32 func_80161104(ForestPlayerState *actor);
s32 func_80161208(ForestPlayerState *actor);
s32 func_801612B8(ForestPlayerState *actor, s32 flags);
s32 func_8016130C(ForestPlayerState *actor, s32 contact);
#endif
