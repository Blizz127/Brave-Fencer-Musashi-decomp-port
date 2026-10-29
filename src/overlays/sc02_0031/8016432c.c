/* Owner re-derivation from retail assembly (clean input set). */
/* func_80164418(o); if func_80146E98(o) returns 0, done. Otherwise o->f4C->f18A = 0,
 * and when o->f4C->f184 is zero, or flags bit 0x400 is set, or f1F8 & 0x80FFFFFF is
 * nonzero, resets f184 to 0; else sets flags bit 0x10 and, when f188 < 0x80, also sets
 * f184 bit 0x80000000. Either way then zeroes f188, clears flags bit 4, and runs
 * func_801553C0(f4C), func_80147364(4, 0x449), func_80147324(0x44B), func_80162CCC(o).
 * void, reached only via jalr table D_80180F30; the sole dispatcher func_80163A58
 * never reads v0 after the jalr. */
#include "psx_types.h"

typedef struct Sub {
    u8 pad0[0x44];
    u32 flags;     /* 0x44 */
    u8 pad48[0x184 - 0x48];
    u32 f184;      /* 0x184 */
    u8 pad188[0x188 - 0x188];
    s16 f188;      /* 0x188 */
    u16 f18A;      /* 0x18A */
    u8 pad18C[0x1F8 - 0x18C];
    u32 f1F8;      /* 0x1F8 */
} Sub;

typedef struct Obj {
    u8 pad0[0x4C];
    Sub *f4C; /* 0x4C */
} Obj;

extern void func_80164418(Obj *o);
extern s32 func_80146E98(Obj *o);
extern void func_801553C0(Sub *s);
extern void func_80147364(s32 a0, s32 a1);
extern void func_80147324(s32 a0);
extern void func_80162CCC(Obj *o);

void func_8016432C(Obj *o)
{
    Sub *s0;
    u32 flags;

    s0 = o->f4C;
    func_80164418(o);

    if (func_80146E98(o) == 0) {
        return;
    }

    s0->f18A = 0;
    if (s0->f184 == 0) {
        goto reset184;
    }

    flags = s0->flags;
    if (flags & 0x400) {
        goto reset184;
    }
    if (s0->f1F8 & 0x80FFFFFF) {
        goto reset184;
    }

    s0->flags = flags | 0x10;
    if (s0->f188 < 0x80) {
        s0->f184 |= 0x80000000;
    }
    s0->f188 = 0;
    goto common;

reset184:
    s0->f184 = 0;
    s0->f188 = 0;

common:
    s0->flags &= ~4;
    func_801553C0(s0);
    func_80147364(4, 0x449);
    func_80147324(0x44B);
    func_80162CCC(o);
}
