/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern u32 D_80126D54;
extern u8 D_801888B0[];

s32 func_8017DCE4(void) {
    return D_80126D54 == (u32)D_801888B0;
}
