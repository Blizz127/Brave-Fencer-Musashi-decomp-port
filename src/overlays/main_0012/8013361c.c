/* func_8013361C [8013361C,801336E8) - owner re-derivation from retail assembly (clean input set). */
/* Box overlap test: boxes a and b (s16 min/max pairs at +4/+6 x, +8/+A y, +C/+E z) offset by
 * oa/ob (x, y, z); returns 1 when they overlap on all three axes, else 0. */
#include "psx_types.h"

/* callers read v0: no direct jal callers; retail sets v0 on every path, so s32. */
s32 func_8013361C(s16 *a, s16 *b, s16 *oa, s16 *ob)
{
    if ((a[2] + oa[0] > b[3] + ob[0]) ||
        (a[3] + oa[0] < b[2] + ob[0]) ||
        (a[6] + oa[2] > b[7] + ob[2]) ||
        (a[7] + oa[2] < b[6] + ob[2]) ||
        (a[4] + oa[1] > b[5] + ob[1]) ||
        (a[5] + oa[1] < b[4] + ob[1])) {
        return 0;
    }
    return 1;
}
