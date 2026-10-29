/* func_80158E24 - owner re-derivation from retail assembly (clean input set).
 * Up to 4 (else 1) attempts of func_80159434(o, 0x4C000); stops when the
 * sub-object's s16 at +0x10 drops under 0xC00 (then clamps it and fires the
 * follow-up calls). Returns the leftover v0 that retail holds (see ledger). */
#include "psx_types.h"
extern u16 func_801487F4(u8 *o);
extern void func_80159434(u8 *o, s32 d);
extern void func_80154274(u8 *o, u8 *p);
extern void func_80154A74(u8 *o, s32 a);
extern void func_8002D4C8(s32 a, s32 b);
extern s32 func_80146CA0(u8 *o);
extern u8 D_800D5904[];

s32 func_80158E24(u8 *o)
{
    s32 n;
    u8 *p;
    s32 d = 0x4C000;
    s32 r;
    s32 lim;
    n = 1;
    r = func_801487F4(o) & 0xF0;
    if (r != 0) {
        n = 4;
    }
    while (n > 0) {
        func_80159434(o, d);
        p = *(u8 **)(o + 0x20);
        lim = 0xC00;
        r = *(s16 *)(p + 0x10) < lim;
        if (r) {
            *(s16 *)(p + 0x10) = lim;
            func_80154274(o, D_800D5904);
            func_80154A74(o, 0x20);
            func_8002D4C8(0x2A, 0);
            func_8002D4C8(0x1C, 0);
            r = func_80146CA0(o);
            n = 0;
        }
        n--;
    }
    return r;
}
