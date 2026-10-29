/* Owner re-derivation from retail assembly and locally generated m2c. */
#include "psx_types.h"

extern s32 D_80078E80;

s32 func_80029524(void) {
    return (u32)(D_80078E80 - 0xC8) < 0x190u;
}
