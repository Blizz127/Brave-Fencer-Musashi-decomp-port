/* Owner re-derivation from retail assembly (clean input set). */
/* Clears voice `voice`'s enable flag in sound slot `slot`. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x3A];
    u8 voice_on[8];
    u8 pad42[0x54 - 0x42];
} SoundSlot;

extern SoundSlot D_800A46E8[];

void func_800342E8(s32 voice, s32 slot) {
    SoundSlot *s;

    s = &D_800A46E8[slot];
    s->voice_on[voice] = 0;
}
