/* Owner re-derivation from retail assembly (clean input set). */
/* Integrates a 3-vector position by its velocity and flags the object when any velocity component is non-zero. */
#include "psx_types.h"

void func_801473EC(u8 *obj) {
    s32 *p = (s32 *)obj;
    p[1] += p[4];
    p[2] += p[5];
    p[3] += p[6];
    if (p[4] != 0 || p[5] != 0 || p[6] != 0) {
        p[0x11] |= 0x80000000;
    }
}
