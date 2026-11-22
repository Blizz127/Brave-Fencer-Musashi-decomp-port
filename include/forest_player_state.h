#ifndef MUSASHI_FOREST_PLAYER_STATE_H
#define MUSASHI_FOREST_PLAYER_STATE_H

#include "psx_types.h"

typedef struct {
    u16 kind, action;
    u8 pad04[0x40];
    u32 flags;
    u8 pad48[4];
    u8 state, phase;
    u8 pad4e[0xdd - 0x4e];
    u8 mode;
} ForestPlayerState;

extern ForestPlayerState D_80126B58;

#if defined(__mips__) && !defined(__LP64__)
extern u8 D_80126BA4;
extern u32 D_8018860C[], D_801891AC[], D_8018925C[];
#define FOREST_CURRENT_STATE D_80126BA4
#define FOREST_PLAYER_CALL(table, index, actor) (((void (*)(ForestPlayerState *))D_##table[index])(actor))
#else
#define FOREST_CURRENT_STATE D_80126B58.state
/* Table entries are four-byte game addresses, including on a 64-bit host. */
u32 bfm_forest_player_table_word(u32 table_address, u32 index);
void bfm_forest_player_call(u32 game_address, ForestPlayerState *actor);
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
#endif
