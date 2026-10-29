/* func_8015B6F4 [8015B6F4,8015B7B4) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

/* callers read v0: yes (bnez v0 at the call site); returns 1 on the taken path, 0 otherwise. */
extern s32 func_80149AA8(u8 *obj);
extern void func_80156648(void *o);
extern void func_80154150(void *o, s32 a);
extern void func_80154A74(u8 *, s32);
extern void func_80146994(s32 a, u8 *o, s32 b, s32 c);
extern void func_80147324(s32 a);
extern void func_80147300(s32 a);
extern u8 D_800D4A9C[];

s32 func_8015B6F4(u8 *o) {
    u8 *p;

    if (func_80149AA8(o) == 0) {
        return 0;
    }
    func_80156648(o);
    *(u8 *)(o + 0x1AA) = 6;
    *(u32 *)(o + 0x44) |= 2;
    *(s32 *)(o + 0x23C) = *(s32 *)(o + 0xB4);
    p = *(u8 **)(o + 0xB4);
    *(u8 *)(o + 0x244) = *(u8 *)(o + 0xBC);
    if (p == D_800D4A9C) {
        func_80154150(o, 0xB);
    } else {
        func_80154150(o, 0xC);
    }
    func_80154A74(o, 0x22);
    *(u8 *)(o + 0xDA) = 0;
    func_80146994(4, o, 5, 0);
    func_80147324(0x400);
    func_80147300(0x5F3);
    return 1;
}
