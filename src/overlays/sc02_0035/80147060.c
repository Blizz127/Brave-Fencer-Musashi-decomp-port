/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

/* Returns retail's leftover v0 (the constant 1 stored to +0x4D). */
s32 func_80147060(u8* o) {
    s32 r = 1;
    *(u8*)(o + 0x4D) = r;
    return r;
}
