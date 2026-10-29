/* Owner re-derivation from retail assembly (clean input set). */
/* Returns the low seven bits of D_80078EC0 when the halfword at offset 0x1C8 is non-zero, otherwise 0. */
#include "psx_types.h"

extern u8 D_80078EC0;

s32 func_801619A4(u8 *obj) {
    s32 r;
    if (*(s16 *)(obj + 0x1C8) != 0) {
        r = D_80078EC0 & 0x7F;
    } else {
        r = 0;
    }
    return r;
}
