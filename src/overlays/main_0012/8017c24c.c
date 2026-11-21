/* Owner re-derivation from retail assembly (clean input set). */
/* The counter wraps to 16 bits while the return retains the full increment. */
#include "psx_types.h"

extern u16 D_80115112;

s32 func_8017C24C(void)
{
    u16 *p;
    s32 result;

    p = &D_80115112;
    result = *p + 1;
    *p = result;
    return result;
}
