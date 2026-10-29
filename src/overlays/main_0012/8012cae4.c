/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern u16 D_801274E4[];
extern s32 func_80016714(u8 *o, s32 a);

/* Returns retail's leftover v0: func_80016714's result from the final call. */
s32 func_8012CAE4(u8 *o) {
    u8 *p;
    D_801274E4[0] |= 1;
    p = *(u8 **)(o + 0x20);
    if (p != 0) {
        if (*(u16 *)p != 1) {
            if (*(u16 *)p == 2) {
                func_80016714(p, 0x38);
            }
        } else {
            func_80016714(p, 0x84);
        }
    }
    return func_80016714(o, 0x10C);
}
