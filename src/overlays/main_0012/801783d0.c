/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

s32 func_801783D0(s32 n, s32 shift) {
    s32 r = 0;
    s32 sh = 0;
    s32 v = n;
    while (v >= 10) {
        n = n / 10;
        r |= (v - ((n << 1) + (n << 3))) << sh;
        v = n;
        sh += 4;
    }
    r |= v << sh;
    return r << shift;
}
