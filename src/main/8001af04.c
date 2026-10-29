/* Owner re-derivation from retail assembly (clean input set). */
/* Maps the mode word D_800BA1B4 to a small code: 0 -> 0, 3 -> 1, otherwise 2. */
#include "psx_types.h"

extern s32 D_800BA1B4;

s32 func_8001AF04(void) {
    s32 mode = D_800BA1B4;

    if (mode == 0) {
        return 0;
    }
    if (mode == 3) {
        return 1;
    }
    return 2;
}
