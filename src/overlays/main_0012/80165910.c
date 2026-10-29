/* Owner re-derivation from retail assembly (clean input set). */
/* Sets the byte flag D_80078EC4 to 1 if it was clear; returns its previous value. */
#include "psx_types.h"

extern u8 D_80078EC4;

s32 func_80165910(void) {
    s32 t = D_80078EC4;
    s32 r = t;
    if (t == 0) {
        D_80078EC4 = 1;
    }
    return r;
}
