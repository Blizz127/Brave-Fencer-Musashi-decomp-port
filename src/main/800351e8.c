/* Owner re-derivation from retail assembly and locally generated m2c. */
#include "psx_types.h"

extern s32 D_80076218;

s32 func_800351E8(s32 arg0) {
    D_80076218 = arg0;
    return arg0 & -(s32)!(0x7EFF0u < (u32)(arg0 - 0x1010));
}
