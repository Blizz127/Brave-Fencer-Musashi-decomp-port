/* Owner re-derivation from retail assembly (clean input set). */
/* Predicate on the s16 state D_80185348: returns 0 only while it equals 1. Referenced as a function
 * pointer (no direct caller). */
#include "psx_types.h"

extern s16 D_80185348;

s32 func_8017A3B0(void)
{
    switch (D_80185348) {
    case 0:
        return 1;
    case 1:
        return 0;
    default:
        return 1;
    }
}
