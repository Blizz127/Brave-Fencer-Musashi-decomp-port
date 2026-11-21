/* Owner re-derivation from retail assembly (clean input set). */
/* Signed 12-bit angle delta, wrapped to +-0x800, optionally scaled by k/t with a minimum step of 1. */
#include "psx_types.h"

s32 func_80012E6C(s16 from, s16 to, s16 t, s16 k, s16 *flag)
{
    s16 d;
    s16 a;
    s32 r;

    to -= from;
    d = to;
    a = d;
    if (*flag == 0 || t == 0) {
        if (to > 0x800) a = to - 0x1000;
        if (a < -0x800) a += 0x1000;
        return a;
    }
    if (to > 0x800) d = to - 0x1000;
    if (d < -0x800) d += 0x1000;
    r = (s16)((d * k) / t);
    if (r == 0) {
        r = -1;
        if (to > 0) r = 1;
    }
    return r;
}
