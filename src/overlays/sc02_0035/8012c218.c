/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern s32 func_80016714(void *o, s32 a);

/* Returns retail's leftover v0: func_80016714's result from the final call. */
s32 func_8012C218(u8 *o) {
    u8 *p = *(u8 **)(o + 0x20);
    if (p != 0) {
        u16 t = *(u16 *)p;
        if (t != 1) {
            if (t == 2) {
                func_80016714(p, 0x38);
            }
        } else {
            func_80016714(p, 0x84);
        }
    }
    return func_80016714(o, 0x10C);
}
