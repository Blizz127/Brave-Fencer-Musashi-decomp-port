/* Owner re-derivation from retail assembly (clean input set). */
/* Returns whether func_80134510(a1) is nonzero. */
#include "psx_types.h"

extern s32 func_80134510(s32 a0);

s32 func_80150150(s32 a0, s32 a1)
{
    return func_80134510(a1) != 0;
}
