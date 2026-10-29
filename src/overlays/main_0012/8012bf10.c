/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
s32 func_8012BF10(u8 *o, s32 n) {
    u32 c = *(u32 *)(o + 0x1C);
    u32 z;
    if (c == 0) {
        z = 1;
    } else {
        c--;
        *(u32 *)(o + 0x1C) = c;
        z = (c == 0);
    }
    if (z) {
        *(u32 *)(o + 0x1C) = n;
        return 1;
    }
    return 0;
}
