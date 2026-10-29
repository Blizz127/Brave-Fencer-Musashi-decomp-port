/* Owner re-derivation from retail assembly (clean input set). */
/* True when dot(a, b) + c is below a threshold chosen from ref[1] (-0x1800 if ref[1] < -0xE00, else -0x2F00). */
#include "psx_types.h"

s32 func_8013435C(s16 *a, s16 *b, s32 c, s16 *ref)
{
    s32 limit;

    limit = -0x2F00;
    if (ref[1] < -0xE00) {
        limit = -0x1800;
    }
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + c < limit;
}
