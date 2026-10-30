/* Owner re-derivation from retail assembly (clean room, clean input set).
 * Stops every playing sequence (state 5) whose id matches: low half of `key` must equal the
 * record's +4 id, and a nonzero high half must also equal its +6 sub-id. For each match it
 * releases the sequence's enabled voices (func_80030D80(channel, 0)) and clears its state.
 * The 0x54-byte records at D_800A46E8: [0..7] sequences, [8..15] voice channels. */
#include "psx_types.h"

typedef struct {
    u16 state;
    u16 pad2;
    u16 id;
    u16 sub;
    u8 pad8[0x3A - 8];
    u8 voice_on[8];
    u8 pad42[0x54 - 0x42];
} SoundRec;

extern SoundRec D_800A46E8[16];
extern void func_80030D80(SoundRec *, s16);

void func_800346D0(s32 key) {
    SoundRec *p;
    s32 i;
    s32 j;

    p = D_800A46E8;
    for (i = 0; i < 8; i++, p++) {
        if (p->state == 5 && p->id == (s16)key && ((key >> 16) == 0 || (key >> 16) == p->sub)) {
            for (j = 0; j < 8; j++) {
                if (p->voice_on[j] != 0) {
                    func_80030D80(&D_800A46E8[j + 8], 0);
                }
            }
            p->state = 0;
        }
    }
}
