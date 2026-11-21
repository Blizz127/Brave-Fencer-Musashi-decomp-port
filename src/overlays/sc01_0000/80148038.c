/* Own retail: suppress outward movement at the threshold before the addition. */
#include "psx_types.h"
void func_80148038(u8 *o, s32 delta)
{
    s32 old = *(s32 *)(o + 0x34);
    if (old < 0) {
        if (delta < 0 && old <= -0x1F0000)
            delta = 0;
    } else {
        if (delta > 0 && old > 0x1EFFFF)
            delta = 0;
    }
    *(u32 *)(o + 0x34) = *(u32 *)(o + 0x34) + (u32)delta;
}
