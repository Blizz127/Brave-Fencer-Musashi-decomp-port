/* Owner re-derivation from retail assembly (clean input set). */
/* Calls a helper with constant 0x2C and returns its low byte. */
#include "psx_types.h"

extern s32 func_800291B4(s32 arg0);

u8 func_80028FBC(void) {
    return func_800291B4(0x2C);
}
