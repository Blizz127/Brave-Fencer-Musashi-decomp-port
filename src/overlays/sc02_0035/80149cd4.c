/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

s32 func_80149CD4(u8 *o) {
    s32 r;
    if (*(u32 *)(o + 0x44) & 0x404) {
        r = 0;
    } else if (*(s16 *)(o + 0x1C8) != 0) {
        r = 0;
    } else {
        r = *(u16 *)(o + 0xAA) & 8;
        r = r != 0;
    }
    return r;
}
