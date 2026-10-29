/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

void func_80148038(u8 *o, s32 d) {
    s32 v = *(s32 *)(o + 0x34);
    if (v < 0) {
        if (d < 0) {
            if (v <= -0x1F0000) {
                d = 0;
            }
        }
    } else {
        if (d > 0) {
            if (v > 0x1EFFFF) {
                d = 0;
            }
        }
    }
    *(s32 *)(o + 0x34) = *(s32 *)(o + 0x34) + d;
}
