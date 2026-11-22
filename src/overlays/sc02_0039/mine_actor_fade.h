#ifndef BFM_MINE_ACTOR_FADE_H
#define BFM_MINE_ACTOR_FADE_H

#include "mine_guest_abi.h"
#include "screen_effect.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
typedef struct {
    u32 active;
    s16 kind;
    u8 reserved06[2];
    u32 flags;
    u8 reserved0c[0x40];
} MineVisualActor;
#else
typedef struct { u8 bytes[0x4c]; } MineVisualActor;
#endif

void func_801379FC(void);
void func_8013A380(void);

#if !BFM_MINE_DIRECT_GUEST_ADDRESS
/* The three built-in records have fixed addresses in this scene member. */
#define BFM_MINE_VISUAL_ACTORS_BASE 0x801269f0u
MineVisualActor *bfm_mine_external_visual_actor(u32 address);
void bfm_mine_update_visual_actor(u32 address, MineVisualActor *actor);
void bfm_mine_draw_visual_actor(u32 address, MineVisualActor *actor);
void bfm_mine_fade_start(ScreenWork *slot);
void bfm_mine_fade_finish(ScreenWork *slot);
/* Returns zero when a focused actor cannot be read. */
int bfm_mine_actor_fade_update(void);
#endif

#endif
