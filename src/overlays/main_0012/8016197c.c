/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

s32 func_8016197C(s32 a, u32 f) {
    s32 r;
    if (f & 1) {
        r = 1;
    } else if (f & 0x4000) {
        r = 2;
    } else {
        r = (f >> 11) & 4;
    }
    return r;
}
