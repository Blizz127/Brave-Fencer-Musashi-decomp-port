/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern s32 func_80016714(u8 *, s32);

/* Returns retail's leftover v0: func_80016714's result from the final call (retail's memset-style helper leaves -1). */
s32 func_80146C3C(u8 *o) {
    u8 *c = *(u8 **)(o + 0x20);
    if (c != 0) {
        func_80016714(c, *(u16 *)c == 1 ? 0x84 : 0x38);
    }
    return func_80016714(o, 0x58);
}
