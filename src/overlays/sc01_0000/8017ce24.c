/* Owner re-derivation from retail assembly (clean input set). */
/* Returns whether the low 16 bits of func_800167F0(0) are non-zero. */
#include "psx_types.h"

extern s32 func_800167F0(s32 a);

s32 func_8017CE24(void)
{
    return (u16)func_800167F0(0) != 0;
}
