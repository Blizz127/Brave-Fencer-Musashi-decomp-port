/* func_801575E4 [801575E4,801576A8) - owner re-derivation from retail assembly (clean input set). */
/* Returns retail's leftover v0: func_80146CA0's result passed up. */
#include "psx_types.h"

/* callers ignore v0, but retail leaves the func_80146CA0 result in it: returned here. */
extern void func_80154A74(u8 *, s32);
extern void func_80154274(void *o, void *a);
extern void func_80147324(s32 a);
extern void func_80146E90(u8 *, s32);
extern void func_80156648(void *o);
extern void func_80146994(s32 a, u8 *o, s32 b, s32 c);
extern void func_8014B768(s32 a0, s32 a1);
extern void func_80147300(s32 a);
extern s32 func_80146CA0(void *o);
extern u8 D_800D4F14[];
extern u32 D_8011F730;

s32 func_801575E4(u8 *o) {
    u8 t;

    func_80154A74(o, 0x15);
    t = *(u8 *)(o + 0x1AB) ^ 1;
    *(u8 *)(o + 0x1AB) = t;
    if (t != 0) {
        *(u8 *)(o + 0x1AA) = 0xA;
    } else {
        *(u8 *)(o + 0x1AA) = 0xB;
    }
    func_80154274(o, D_800D4F14);
    func_80147324(0x455);
    func_80146E90(o, 0);
    func_80156648(o);
    func_80146994(5, o, 0, 0);
    func_8014B768((s32)o, 0x333);
    func_80147300(0x5F3);
    D_8011F730 |= 4;
    return func_80146CA0(o);
}
