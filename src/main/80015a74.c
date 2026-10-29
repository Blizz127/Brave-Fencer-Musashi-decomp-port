/* Owner re-derivation from retail assembly (clean input set). */
/* Converts a binary value to packed BCD, ten decimal digits, most significant first. */
#include "psx_types.h"

u32 func_80015A74(u32 n)
{
    u32 div = 1000000000;
    u32 r = 0;
    s32 i;

    for (i = 0; i < 10; i++) {
        r = (r << 4) | (n / div);
        n %= div;
        div /= 10;
    }
    return r;
}
