/* Owner re-derivation from retail assembly (clean input set). */
/* Splits the segment a->b into thirds: c = b - step and b = a + step (each endpoint flagged +6 when the step is zero or too short). */
#include "psx_types.h"

extern s32 func_80013294(void *a, void *b);

void func_80156FA8(s16 *a, s16 *b, s16 *c) {
    s16 d[3];
    d[0] = (b[0] - a[0]) / 3;
    d[1] = (b[1] - a[1]) / 3;
    d[2] = (b[2] - a[2]) / 3;
    a[3] = 0;
    if (func_80013294(a, b) > 0x40) {
        if (d[0] != 0 || d[1] != 0 || d[2] != 0) {
            c[0] = b[0] - d[0];
            c[1] = b[1] - d[1];
            c[2] = b[2] - d[2];
            c[3] = 0;
        } else {
            c[3] = 1;
        }
        if (d[0] != 0 || d[1] != 0 || d[2] != 0) {
            b[0] = a[0] + d[0];
            b[1] = a[1] + d[1];
            b[2] = a[2] + d[2];
            b[3] = 0;
        } else {
            b[3] = 1;
        }
    } else {
        c[3] = 1;
        b[3] = 1;
    }
}
