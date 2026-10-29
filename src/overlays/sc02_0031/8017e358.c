/* Owner re-derivation from retail assembly (clean input set). */
/* Copies three words from (o+0x4C)->+4/+8/+0xC into o+4/8/0xC, then adds the halfword at
 * (o+0x58)->+6 into the halfword at (o+0x20)->+0x12. */
#include "psx_types.h"

void func_8017E358(u8 *o)
{
    u8 *src = *(u8 **)(o + 0x4C);
    u8 *dst = *(u8 **)(o + 0x20);
    *(s32 *)(o + 4) = *(s32 *)(src + 4);
    *(s32 *)(o + 8) = *(s32 *)(src + 8);
    *(s32 *)(o + 0xC) = *(s32 *)(src + 0xC);
    {
        u8 *p = *(u8 **)(o + 0x58);
        u16 v = *(u16 *)(dst + 0x12);
        u16 w = *(u16 *)(p + 0x6);
        *(u16 *)(dst + 0x12) = (u16)(v + w);
    }
}
