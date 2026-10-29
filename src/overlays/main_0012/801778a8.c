/* Owner re-derivation from retail assembly (clean input set). */
/* Writes cnt 0x14-byte entries after o: a size byte from successive nibbles of
 * the low halfword of a1 and bit 0x100 of the u16 flags, cleared from the first
 * non-zero nibble, the last entry or entry `mark` on (form of func_80177AD4). */
#include "psx_types.h"

typedef struct { u8 pad0[0xA]; u16 f; u8 b; u8 pad1[7]; } Ent;

/* Callers do not read v0. */
void func_801778A8(Ent *o, s32 a1, s16 cnt, s16 mark) {
    s16 i;
    s32 flag;
    s32 hv;
    s16 n;
    flag = 0x100;
    for (i = 0; i < cnt; ) {
        n = (u32)(a1 << 16) >> 28;
        if (n != 0 || i == cnt - 1 || i == mark) {
            flag = 0;
        }
        a1 <<= 4;
        i++;
        hv = o->f;
        o->f = flag | (hv & ~0x100);
        o->b = n * 8 + 8;
        o++;
    }
}
