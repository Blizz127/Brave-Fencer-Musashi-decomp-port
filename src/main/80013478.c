/* Owner re-derivation from retail assembly (clean room, clean input set). */
/* Sum of squared per-axis differences between two position vectors, via the GTE square op. */
#include "psx_types.h"
#include "gte_ops.h"

s32 func_80013478(s16 *a, s16 *b) {
    s32 d[3];
    s32 sq[3];

    d[0] = a[1] - b[0];
    d[1] = a[3] - b[1];
    d[2] = a[5] - b[2];
    gte_lwc2(9, 0, &d[0]);
    gte_lwc2(10, 4, &d[0]);
    gte_lwc2(11, 8, &d[0]);
    gte_nop();
    gte_nop();
    gte_sqr0();
    gte_swc2(25, 0, &sq[0]);
    gte_swc2(26, 4, &sq[0]);
    gte_swc2(27, 8, &sq[0]);
    return sq[0] + sq[1] + sq[2];
}
