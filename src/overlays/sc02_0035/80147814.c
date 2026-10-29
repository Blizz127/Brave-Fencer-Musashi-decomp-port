/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

void func_80147814(u8 *o, s32 a) {
    *(s32 *)(o + 0x34) += a;
    if (a < 0 && *(s32 *)(o + 0x34) < -0x1F0000) {
        *(s32 *)(o + 0x34) = -0x1F0000;
    } else if (a > 0 && *(s32 *)(o + 0x34) > 0x1F0000) {
        *(s32 *)(o + 0x34) = 0x1F0000;
    }
}
