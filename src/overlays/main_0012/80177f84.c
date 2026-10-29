/* func_80177F84 - owner re-derivation from retail assembly (clean input set).
 * Writes five 0x14-byte entries after o: a size byte from successive nibbles
 * of a1 (from bit 16 down) and bit 0x100 of the u16 flags, cleared from the
 * first non-zero nibble on. */
#include "psx_types.h"

typedef struct { u8 pad0[0xA]; u16 f; u8 b; u8 pad1[7]; } Ent;

void func_80177F84(Ent *o, s32 a1) {
    s16 i;
    s32 flag;
    s32 hv;
    s16 n;
    s32 m;
    flag = 0x100;
    i = 0;
    do {
        n = (a1 >> 16) & 0xF;
        m = n;
        a1 <<= 4;
        if (i == 5 || m != 0) {
            flag = 0;
        }
        o++;
        i++;
        hv = o->f;
        o->f = flag | (hv & ~0x100);
        o->b = n * 8 + 8;
    } while (i < 5);
}
