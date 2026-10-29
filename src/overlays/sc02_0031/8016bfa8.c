/* Owner re-derivation from retail assembly (clean input set). */
/* Tail-forwards to func_8016BFD0, appending &D_801B6938 as its 5th (stack) argument;
 * a0-a3 are forwarded unchanged from this function's own incoming registers. */
#include "psx_types.h"

extern s32 D_801B6938;
extern s32 func_8016BFD0(s32 a0, void *a1, void *a2, s32 a3, void *a4);

s32 func_8016BFA8(s32 a0, void *a1, void *a2, s32 a3)
{
    return func_8016BFD0(a0, a1, a2, a3, &D_801B6938);
}
