/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
extern void func_80146E90(u8 *o, s32 a);
extern void func_80146CA0(u8 *o);
extern void func_80165124(u8 *o);
extern void func_80165140(u8 *o);
extern void func_801651B8(u8 *o);

void func_80164F44(u8 *o)
{
    u16 t;
    u16 c;
    s32 k;

    *(u16 *)(o + 0x62) += 4;
    *(u16 *)(o + 0x60) += 0x40;
    t = *(u16 *)(o + 0x2E) - *(u16 *)(o + 0x60);
    *(u16 *)(o + 0x2E) = t;
    *(u16 *)(o + 0x2C) = t;
    if ((s16)t < 0x1000) {
        *(u16 *)(o + 0x2E) = 0x1000;
        *(u16 *)(o + 0x2C) = 0x1000;
        *(u16 *)(o + 0x60) = 0x80;
        func_80146E90(o, 0xE);
        func_80146CA0(o);
    }
    func_80165124(o);
    if (*(s16 *)(o + 0x2C) < 0x2800) {
        c = *(u16 *)(o + 0x66) + 1;
        k = 2;
    } else {
        c = *(u16 *)(o + 0x66) + 1;
        k = 3;
    }
    *(u16 *)(o + 0x66) = c;
    if ((s16)c == k) {
        func_80165140(o);
        *(u16 *)(o + 0x66) = 0;
    }
    func_801651B8(o);
}
