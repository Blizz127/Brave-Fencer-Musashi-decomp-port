/* Owner re-derivation from retail assembly (clean input set). */
/* Returns whether the low 16 bits of func_800167F0(0) are nonzero. Same body as the scene twins (sc03_0001 func_8017E4E4). */
#include "psx_types.h"

extern s32 func_800167F0(s32 a);

s32 func_800D2F8C(void)
{
    return (u16)func_800167F0(0) != 0;
}
