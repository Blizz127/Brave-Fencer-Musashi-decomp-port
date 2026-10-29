/* Owner re-derivation from retail assembly (clean input set). */
/* Same timer-decrement shape as sibling func_8017D2A8: if the s16-truncated result is > 0,
 * calls func_8017D2F4(o); else sets the three u16 fields at (o+0x34)->+0x64/+0x62/+0x60 to
 * 0x1000 and calls func_80146C3C(o). */
#include "psx_types.h"

extern void func_8017D2F4(u8 *o);
extern void func_80146C3C(u8 *o);

void func_8017D84C(u8 *o)
{
    s32 v = *(u16 *)(o + 0x12) - 0x100;
    u8 *rec = *(u8 **)(o + 0x34);
    *(s16 *)(o + 0x12) = (s16)v;
    if ((s16)v > 0) {
        func_8017D2F4(o);
    } else {
        *(u16 *)(rec + 0x64) = 0x1000;
        *(u16 *)(rec + 0x62) = 0x1000;
        *(u16 *)(rec + 0x60) = 0x1000;
        func_80146C3C(o);
    }
}
