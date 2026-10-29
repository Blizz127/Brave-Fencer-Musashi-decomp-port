/* Owner re-derivation from retail assembly (clean input set). */
/* Nonzero when func_800167F0(4) has any of its low 16 bits set. */
#include "psx_types.h"

extern s32 func_800167F0(s32 a);

s32 func_800D319C(void)
{
    return (func_800167F0(4) & 0xFFFF) != 0;
}
