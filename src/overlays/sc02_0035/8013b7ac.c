/* Owner re-derivation from retail assembly (clean input set). */
/* Zeroes D_801BE920[a0 * 7]. */
#include "psx_types.h"

extern s32 D_801BE920[];

void func_8013B7AC(s32 a0)
{
    *(s32 *)((u8 *)D_801BE920 + ((a0 * 7) << 2)) = 0;
}
