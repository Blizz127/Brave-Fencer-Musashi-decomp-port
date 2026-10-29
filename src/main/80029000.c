/* Owner re-derivation from retail assembly (clean input set). */
/* Returns the low byte of func_800291B4(0x2D). */
#include "psx_types.h"

extern s32 func_800291B4(s32);

u8 func_80029000(void) {
    return func_800291B4(0x2D);
}
