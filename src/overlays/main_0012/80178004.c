/* Owner re-derivation from retail assembly (clean input set). */
/* Draws the six fixed gauge segments (inlined func_80177784 bodies at x+0,7,0xD,0x13,0x19,0x3A) and the three-nibble readout strip sized from D_8011F81E; returns the next free packet.
 * callers read v0: yes (801761C8, 80176D70 chain it), so Pr *. Callers sign-extend both a1 and a2; y is s16,
 * x stays s32 (an s16 x adds re-masking; owner 80176d00.c declares (u32, s16, s16) s32). The readout loop is
 * func_801777BC's body as a static inline helper (n=3, c=0xB4, k=0xFF); its s16 y param adds the loop RTL
 * that keeps 0x74808080 unhoisted, and t is set before flag. */
#include "psx_types.h"

typedef struct {
    u32 tag;
    u32 a;
    u32 b;
    u32 c;
    u32 d;
} Pr;

extern s16 D_8011F81E;
extern s32 func_801783D0(s32 n, s32 shift);

static inline Pr *seg(Pr *p, u32 xy, u32 k) {
    p->tag = (((u32)p - 0x14) & 0xFFFFFF) | 0x3000000;
    p->a = 0x74808080;
    p->b = xy;
    p->c = k;
    return p + 1;
}

static inline Pr *strip(Pr *p, u32 a1, u32 x, s16 y, s16 n, s16 c, s32 k)
{
    s16 i;
    u32 flag;
    s16 w;
    u32 t;
    u32 v;
    u32 e;

    t = (((c + 0x100) << 6) | 0x16) << 16;
    flag = 0x1000000;
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

Pr *func_80178004(Pr *p, s32 x, s16 y) {
    u32 hy = y << 16;
    u32 a1;
    p = seg(p, hy | (x & 0xFFFF), 0x6D165828);
    p = seg(p, hy | ((x + 7) & 0xFFFF), 0x6D160048);
    p = seg(p, hy | ((x + 0xD) & 0xFFFF), 0x6D160818);
    p = seg(p, hy | ((x + 0x13) & 0xFFFF), 0x6D160028);
    p = seg(p, hy | ((x + 0x19) & 0xFFFF), 0x6D160020);
    p = seg(p, hy | ((x + 0x3A) & 0xFFFF), 0x6D164058);
    a1 = func_801783D0(0x64 - ((D_8011F81E * 41368) >> 16), 4);
    p = strip(p, a1, x + 0x22, y, 3, 0xB4, 0xFF);
    return p;
}
