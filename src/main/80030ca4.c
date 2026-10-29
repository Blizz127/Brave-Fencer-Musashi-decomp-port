/* Owner re-derivation from retail assembly (clean input set). */
/* Guest pointers held as u32 for the 64-bit native lane. */
/* Finds a free sound channel (returns channel+1); otherwise releases the lowest-priority one if its priority does not exceed `priority`, or returns 0. */
#include "psx_types.h"

typedef struct {
    u8 pad0[8];
    u16 priority;
    u8 padA[0x4E - 0xA];
    u8 active;
    u8 pad4F[0x54 - 0x4F];
} SoundChannel;

typedef struct {
    s32 index;
    u32 channel; /* guest SoundChannel * */
    u16 priority;
    u8 found;
} ChannelChoice;

extern void func_80030D80(SoundChannel *, s32);
extern SoundChannel D_800A4988[8];

s32 func_80030CA4(s32 priority) {
    ChannelChoice best;
    SoundChannel *ch;
    s32 i;

    best.found = 0;
    for (ch = D_800A4988, i = 0; i < 8; i++, ch++) {
        if (ch->active == 0) {
            return i + 1;
        }
        if (best.found == 0) {
            best.priority = ch->priority;
            best.found = 1;
            best.index = i;
            best.channel = (u32)ch;
        } else if (ch->priority < best.priority) {
            best.priority = ch->priority;
            best.index = i;
            best.channel = (u32)ch;
        }
    }
    if ((u16)priority < best.priority) {
        return 0;
    }
    func_80030D80((SoundChannel *)best.channel, 1);
    ch->active = 0;
    return ++best.index;
}
