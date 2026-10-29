/* Owner re-derivation from retail assembly (clean input set). */
/* Releases (func_80030D80) every slot used by a sequence's enabled voices with the given mode, then clears the sequence's state word. */
#include "psx_types.h"

typedef struct {
    u8 data[0x54];
} SoundSlot;

typedef struct {
    s16 state;
    u8 pad2[0x3A - 2];
    u8 voice_on[8];
} SeqVoices;

extern void func_80030D80(SoundSlot *, s16);
extern SoundSlot D_800A4988[8];

void func_80034650(SeqVoices *seq, u8 mode) {
    s32 i;

    for (i = 0; i < 8; i++) {
        if (seq->voice_on[i] != 0) {
            func_80030D80(&D_800A4988[i], mode);
        }
    }
    seq->state = 0;
}
