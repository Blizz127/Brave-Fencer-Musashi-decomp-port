/* func_801777BC - owner re-derivation from retail assembly (clean input set).
 * Builds n 0x14-byte flat primitives linked backwards, sizes from nibbles
 * of a1; returns the next free primitive (callers read v0). p stays the
 * parameter so the strength-reduced address starts from a0. */
#include "psx_types.h"
typedef struct { u32 tag; u32 a; u32 b; u32 c; u32 d; } Pr801777BC;

Pr801777BC *func_801777BC(Pr801777BC *p, u32 a1, u32 x, s32 y, s16 n, s16 c, s32 k)
{
    s16 i;
    u32 flag;
    s16 w;
    u32 t;
    u32 v;
    u32 e;

    flag = 0x1000000;
    t = (((c + 0x100) << 6) | 0x16) << 16;
    for (i = 0; i < n; x += 8, i++, a1 <<= 4) {
        w = (a1 << 16) >> 28;
        if (w != 0 || i == n - 1 || i == (s16)k) {
            flag = 0;
        }
        p->tag = ((u32)(p - 1) & 0xFFFFFF) | 0x03000000;
        v = (y << 16) | (x & 0xFFFF) | flag;
        p->a = 0x74808080;
        p->b = v;
        e = ((w << 3) + 8) | 0x4000;
        p->c = t | e;
        p++;
    }
    return p;
}
