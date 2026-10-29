/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern s32 func_80131CF4(u32 p, s32 key);
extern void func_8002D4C8(s32, s32);

/* key (a1) is forwarded untouched to func_80131CF4 (retail reads entry $a1). */
u16 func_80131AC8(u8 *o, s32 key) {
    u16 r = func_80131CF4(*(u32 *)(o + 0xBC), key);
    if (r != 0) {
        func_8002D4C8(r, 0);
        return r;
    }
    return 0;
}
