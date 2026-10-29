/* Owner re-derivation from retail assembly (clean input set). */
/* Tail-forwards to func_80146A6C(0x19, o, 0, 0, 0, 0, 1); its s32 result becomes this
 * function's return, matching the sibling func_8016634C/func_8016BA68 call shape. */
#include "psx_types.h"

extern s32 func_80146A6C(s32 a, void *o, s32 x, s32 y, s32 z, s32 c1, s32 c2);

s32 func_8017CF98(void *o)
{
    return func_80146A6C(0x19, o, 0, 0, 0, 0, 1);
}
