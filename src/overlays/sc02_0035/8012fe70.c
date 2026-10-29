/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern s32 func_80131E00(u8 *o, s32 a);
extern void func_80131170(u8 *o, s32 a, s32 b);
extern s32 func_80131CA8(u8 *o, s32 a);

/* Returns retail's leftover v0: func_80131E00's result when bit 1 of the flags is set, else func_80131CA8's result. */
s32 func_8012FE70(u8 *o) {
    u32 fl = *(u32 *)(o + 0xC4);
    s32 r;
    o[0xC1] = 9;
    if (fl & 2) {
        o[0xC1] = 1;
        r = func_80131CA8(o, 3);
    } else if (fl & 1) {
        r = func_80131E00(o, 1);
    } else {
        if (*(u32 *)(o + 0xB4) & 0x200) {
            func_80131170(o, 0, 0);
        }
        r = func_80131CA8(o, 0x1E);
    }
    return r;
}
