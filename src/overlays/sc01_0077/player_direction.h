#ifndef SC01_0077_PLAYER_DIRECTION_H
#define SC01_0077_PLAYER_DIRECTION_H

#include "psx_types.h"

/* Camera target; only the position words used here. */
typedef struct {
    u8 pad00[0x5C];
    s32 x;
    u8 pad60[4];
    s32 z;
    s32 target_x;
    u8 pad6C[4];
    s32 target_z;
} SceneDirectionCamera;

/* Linked object; only its heading. */
typedef struct {
    u8 pad00[0x12];
    s16 heading;
} SceneDirectionLink;

typedef struct {
    u8 pad00[0x20];
    u32 link;
    u8 pad24[0xA9 - 0x24];
    u8 direction_mode;
    u16 direction_bits;
    u8 padAC[0xB8 - 0xAC];
    u16 animation_status;
    u8 padBA[0x198 - 0xBA];
    u32 effect_address;
    u8 pad19C[0x20C - 0x19C];
    union {
        u8 counter;
        s16 phase;
    } event_progress;
    u8 pad20E[0x214 - 0x20E];
    u8 event_state;
} ScenePlayer;

s32 func_80171B4C(ScenePlayer *player, s32 frames);
s32 func_8017E170(ScenePlayer *player);

#endif
