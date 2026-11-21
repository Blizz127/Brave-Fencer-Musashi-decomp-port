/* Owner re-derivation from retail assembly (clean input set). */
/* Scans ids 0x2F..0x3A, returning the first i whose func_800291B4(i)&0xFF equals id&0xFF;
 * returns 0 if none match (do-while: no upfront bound check before the first call). */
#include "psx_types.h"

extern u8 func_800291B4(s32 id);

s32 func_800D0F8C(s32 id)
{
    u32 i = 0x2F;
    u8 target = (u8)id;
    do {
        if ((func_800291B4(i) & 0xFF) == target) {
            return i;
        }
        i++;
    } while (i < 0x3B);
    return 0;
}
