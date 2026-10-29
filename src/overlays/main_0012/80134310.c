/* Owner re-derivation from retail assembly (clean input set). */
/* Plane test value: dot(a, b) of two s16 3-vectors plus offset c. */
#include "psx_types.h"

s32 func_80134310(s16 *a, s16 *b, s32 c)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + c;
}
