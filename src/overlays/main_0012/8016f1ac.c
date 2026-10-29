/* Owner re-derivation from retail assembly (clean input set). */
/* Returns the global word D_80126D50 with bits 24-30 cleared (callers test it for zero). */
#include "psx_types.h"

extern u32 D_80126D50;

u32 func_8016F1AC(void)
{
    return D_80126D50 & 0x80FFFFFF;
}
