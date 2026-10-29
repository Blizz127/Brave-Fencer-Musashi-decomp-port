/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
extern void func_80049CAC(u8 *a, u8 *b);

void func_8012E8E0(u8 *o, u16 *s)
{
    u8 *q;
    u8 *p;
    u8 *r;
    u16 v;

    p = *(u8 **)(o + 0x20);
    *(u16 *)(o + 6) = s[0];
    *(u16 *)(o + 0xA) = s[1];
    *(u16 *)(o + 0xE) = s[2];
    *(u16 *)(o + 0xC) = 0;
    *(u16 *)(o + 8) = 0;
    *(u16 *)(o + 4) = 0;
    if (p != 0) {
        *(u16 *)(p + 0x12) = s[3];
        q = *(u8 **)(o + 0x20);
        if (q != 0) {
            func_80049CAC(q + 0x10, q + 0x34);
            r = *(u8 **)(o + 0x20);
            v = *(u16 *)(o + 6) + *(u16 *)(o + 0x50);
            *(u16 *)(r + 8) = v;
            *(s32 *)(r + 0x48) = (s16)v;
            v = *(u16 *)(o + 0xA) + *(u16 *)(o + 0x52);
            *(u16 *)(r + 0xA) = v;
            *(s32 *)(r + 0x4C) = (s16)v;
            v = *(u16 *)(o + 0xE) + *(u16 *)(o + 0x54);
            *(u16 *)(r + 0x2C) |= 1;
            *(u16 *)(r + 0xC) = v;
            *(s32 *)(r + 0x50) = (s16)v;
        }
    }
}
