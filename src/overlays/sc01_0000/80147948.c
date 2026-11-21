/* Own retail: wrapping increment, then sign-directed bounds on the wrapped value. */
#include "psx_types.h"
void func_80147948(u8 *o, s32 delta, s32 limit)
{
    s32 value = (s32)(*(u32 *)(o + 0x24) + (u32)delta);
    *(s32 *)(o + 0x24) = value;
    if (delta < 0) {
        s32 lower = (s32)(0U - (u32)limit);
        if (value <= lower) {
            *(s32 *)(o + 0x24) = lower;
            return;
        }
    }
    if (delta > 0 && *(s32 *)(o + 0x24) >= limit)
        *(s32 *)(o + 0x24) = limit;
}
