/* Owner re-derivation from retail assembly (clean input set). */
/* Steps byte +6 down by byte +5, decrements +5, writes the new extent into the
 * six halfwords at +0xC..+0x16, calls func_801757A0 with the current id and,
 * when +5 reaches zero, clears +8 and advances the phase byte +1. */
#include "psx_types.h"

extern u16 D_800B9A02;
extern void func_801757A0(s32 id);

/* Reached through a table; void. */
void func_80175590(u8 *o) {
    u8 a;
    u8 d;
    s16 v;
    s16 n;
    a = o[5];
    d = o[6] - a;
    n = -d;
    o[6] = d;
    o[5] = a + 0xFF;
    v = o[6];
    *(s16 *)(o + 0x10) = n;
    *(s16 *)(o + 0xC) = n;
    *(s16 *)(o + 0x16) = v;
    *(s16 *)(o + 0x12) = v;
    *(s16 *)(o + 0xE) = v;
    *(s16 *)(o + 0x14) = v;
    func_801757A0((s16)D_800B9A02);
    if (o[5] == 0) {
        o[8] = 0;
        o[1] = o[1] + 1;
    }
}
