/* func_80177AD4 - owner re-derivation from retail assembly (clean input set).
 * Four-entry sibling of func_80177F84: nibbles of the low halfword of a1. */
#include "psx_types.h"

typedef struct { u8 pad0[0xA]; u16 f; u8 b; u8 pad1[7]; } Ent;

void func_80177AD4(Ent *o, s32 a1) {
    s16 i;
    s32 flag;
    s32 hv;
    s16 n;
    flag = 0x100;
    i = 0;
    o++;
    do {
        n = (u32)(a1 << 16) >> 28;
        if (n != 0 || i == 3 || i == 1) {
            flag = 0;
        }
        a1 <<= 4;
        i++;
        hv = o->f;
        o->f = flag | (hv & ~0x100);
        o->b = n * 8 + 8;
        o++;
    } while (i < 4);
}
