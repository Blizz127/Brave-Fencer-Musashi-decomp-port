/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"
extern u8 D_801151F0[];

s32 func_8014C918(s32 unused, s32 k)
{
    u8 i;
    if (k == 0x18) {
        return 8;
    }
    for (i = 0; i < 8; i++) {
        if (D_801151F0[i] == k) {
            return i;
        }
    }
    return 0;
}
