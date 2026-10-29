/* Owner re-derivation from retail assembly (clean input set). */
/* Computes the 12-bit angle from the second point to the first, minus 0x400. */
#include "psx_types.h"

extern s32 func_8004CFEC(s32 dx, s32 dy);

s32 func_8012B744(s16 *a, s16 *b) {
    s32 r = func_8004CFEC(a[5] - b[2], b[0] - a[1]);
    return (r - 0x400) & 0xFFF;
}
