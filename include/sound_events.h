#ifndef BFM_SOUND_EVENTS_H
#define BFM_SOUND_EVENTS_H

#include "psx_types.h"

typedef struct {
    u8 kind;
    u8 reserved[2];
    /* Entry kind selects the interpretation of the fourth byte. */
    u8 selector;
} BfmSoundEntry;

typedef struct {
    u16 kind;
    u16 reserved2;
    u16 id;
    u16 sub_id;
    u8 reserved8[6];
    u8 voice_used[8];
    u8 reserved16[10];
    u32 pitch;
    u8 reserved24[4];
    u32 pitch_target;
    s32 pitch_step;
    s16 pitch_timer;
    u8 reserved32[5];
    u8 flags;
    u8 pitch_done;
    u8 reserved39[27];
} BfmSoundChannel;

s32 func_8002DC68(s32 id, u16 argument);
void func_8002F248(BfmSoundEntry *event, s32 id);
s32 func_800331D4(u32 value);

#endif
