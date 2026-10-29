/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern s32 func_80131CA8(void *, s32);

/* Returns retail's leftover v0: func_80131CA8's result from the final call. */
s32 func_8012FF4C(u8 *o) {
    o[0xC1] = 5;
    if (*(u32 *)(o + 0xB4) & 0x20) {
        u8 *p = *(u8 **)(o + 0x20);
        *(u16 *)(o + 0x5C) = 0;
        *(s32 *)(o + 0x1C) = 0x1E;
        *(u16 *)(o + 0x98) = 0;
        *(u16 *)(p + 0x10) = 0;
    }
    return func_80131CA8(o, 0xF);
}
