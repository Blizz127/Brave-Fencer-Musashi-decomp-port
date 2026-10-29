/* Owner re-derivation from retail assembly (clean input set). */
/* Returns retail's leftover v0 (see ledger). */
#include "psx_types.h"
extern u8 D_800D52E8[];
extern s32 func_80161208(u8 *o);
extern s32 func_801585AC(u8 *o);
extern void func_80156648(u8 *o);
extern void func_80154274(u8 *o, u8 *p);
extern void func_80147324(s32 a);
extern void func_80146994(s32 a, u8 *o, s32 b, s32 c);
extern void func_80146E90(u8 *o, s32 a);
extern void func_80146CA0(u8 *o);
extern void func_801599A4(u8 *o);
extern s32 func_80159B3C(u8 *o);
extern s32 func_80155FF8(u8 *o, s32 a);

s32 func_80158344(u8 *o)
{
    s32 pad[4];
    s32 r;
    *(u32 *)(o + 0x44) |= 2;
    r = func_80161208(o);
    if (r != 0) {
        return r;
    }
    if (func_801585AC(o) != 0) {
        func_80156648(o);
        o[0x1AA] = 0x11;
        func_80154274(o, D_800D52E8);
        *(u32 *)(o + 0x234) = 0;
        func_80147324(0x455);
        func_80146994(4, o, 9, 0);
        func_80146994(5, o, 0xD, 0);
        func_80146E90(o, 0x12C);
        func_80146CA0(o);
    }
    if ((*(u16 *)(o + 0xB8) & 0x8000) != 0) {
        func_801599A4(o);
        r = func_80159B3C(o);
    } else {
        r = func_80155FF8(o, o[0x1AA]);
    }
    return r;
}
