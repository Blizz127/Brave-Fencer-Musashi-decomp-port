/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern int func_8012BEE8(u8 *o);

int func_80142D38(u8 *o) {
    if (func_8012BEE8(o) == 1) {
        return 1;
    }
    if (*(u32 *)(o + 0x1C) & 1) {
        *(u32 *)(*(u8 **)(o + 0x20) + 4) |= 0x80000000;
    } else {
        *(u32 *)(*(u8 **)(o + 0x20) + 4) &= 0x7FFFFFFF;
    }
    return 0;
}
