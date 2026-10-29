/* Owner re-derivation from retail assembly (clean input set). */
/* Decrements the counter at +0x1C; once it reaches <= 0, clears the sign bit of the word
 * at (+0x20)->+0x4 and sets the halfword flag at +0x2. */
#include "psx_types.h"

void func_80182BA0(u8 *a1)
{
    s32 c = *(s32 *)(a1 + 0x1C) - 1;
    *(s32 *)(a1 + 0x1C) = c;
    if (c > 0) {
        return;
    }
    {
        u8 *o2 = *(u8 **)(a1 + 0x20);
        s32 v = *(s32 *)(o2 + 4);
        v &= 0x7FFFFFFF;
        *(s32 *)(o2 + 4) = v;
    }
    *(s16 *)(a1 + 2) = 1;
}
