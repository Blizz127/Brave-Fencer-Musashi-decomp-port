/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern void func_80131B14(u8 *o, s32 a1);
extern s32 func_80131E00(u8 *o, s32 a);
extern s32 func_80131A34(u8 *o, s32 a);
extern s32 func_80131CA8(u8 *o, s32 a);

/* a1 is forwarded untouched to func_80131B14 (retail reads entry $a1). */
/* Returns retail's leftover v0: the result of whichever final call ran (func_80131CA8 or func_80131E00). */
s32 func_8012F5F4(u8 *o, s32 a1) {
    o[0xC1] = 1;
    if (*(u32 *)(o + 0xC4) & 2) {
        return func_80131CA8(o, 3);
    }
    func_80131B14(o, a1);
    if (*(s16 *)(o + 0x76) <= 0) {
        return func_80131E00(o, 0xC);
    }
    if (func_80131A34(o, 4) != 0) {
        o[0xC2] = 0;
    } else {
        *(u16 *)(o + 0x98) = 0;
        o[0xC2] = 1;
        *(u32 *)(o + 0x1C) = 0x10;
    }
    return func_80131CA8(o, 3);
}
