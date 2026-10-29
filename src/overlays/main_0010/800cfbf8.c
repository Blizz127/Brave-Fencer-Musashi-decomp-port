/* Owner re-derivation from retail assembly (clean input set). */
/* Maps the class of the current mode word (func_800CFC5C of D_800B9A08) to a replacement mode id: 2 -> 0x1052, 3 -> 0x1077, 4 -> 0x1094, else 0x1010. */
#include "psx_types.h"

extern s32 func_800CFC5C(s32 id);
extern s16 D_800B9A08;

s16 func_800CFBF8(void)
{
    s32 c;

    c = func_800CFC5C(D_800B9A08);
    if (c == 1) {
        return 0x1010;
    }
    if (c == 2) {
        return 0x1052;
    }
    if (c == 3) {
        return 0x1077;
    }
    if (c == 4) {
        return 0x1094;
    }
    return 0x1010;
}
