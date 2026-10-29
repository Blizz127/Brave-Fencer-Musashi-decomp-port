/* Owner re-derivation from retail assembly (clean input set). */
/* Decrements the u16 timer at o+0x12 by 0x100 (stored back truncated); if the s16-truncated
 * result is <= 0, calls func_80146C3C(o), else func_8017D2F4(o). */
#include "psx_types.h"

extern void func_8017D2F4(u8 *o);
extern void func_80146C3C(u8 *o);

void func_8017D2A8(u8 *o)
{
    s32 v = *(u16 *)(o + 0x12) - 0x100;
    *(s16 *)(o + 0x12) = (s16)v;
    if ((s16)v > 0) {
        func_8017D2F4(o);
    } else {
        func_80146C3C(o);
    }
}
