#ifndef GAME_TRACK_STATE_H
#define GAME_TRACK_STATE_H

#include "psx_types.h"

/* Five bank slots are initialized at 24-byte intervals by the sound driver. */
typedef struct {
    s32 address;
    s16 track;
    s16 handle;
    s16 other_type;
    s16 reserved_a;
    s16 other_track;
    s16 reserved_e;
    s32 header;
    u8 empty;
    u8 pad[3];
} TrackSlot;
typedef struct {
    s32 tick;
    TrackSlot slots[5];
    u8 tail_7c[8];
    s32 header;
    u8 tail_88[12];
    s16 phase;
    s16 slot;
    s16 track;
    s16 handle;
} TrackControl;
extern TrackControl D_800A4638;

extern s16 func_80041A80(s32 header, s32 bank, s32 address);

#endif
