/* Owner re-derivation from retail assembly (clean input set). */
/* Returns retail's leftover v0 (see ledger). */
#include "psx_types.h"

extern u16 func_801487F4(u8 *o);
extern s32 func_80146E98(u8 *o);
extern void func_80147324(s32 a);
extern s32 func_80146CA0(u8 *o);

s32 func_80158CD8(u8 *o) {
    s32 n;
    s32 r;
    n = 1;
    r = func_801487F4(o) & 0xF0;
    if (r) {
        n = 4;
    }
    for (; n > 0; n--) {
        r = func_80146E98(o);
        if (r != 0) {
            func_80147324(0x453);
            *(u32 *)(o + 0x238) = 0x5B0000;
            r = func_80146CA0(o);
            n = 0;
        }
    }
    return r;
}
