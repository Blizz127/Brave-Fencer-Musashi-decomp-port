/* Owner re-derivation from retail assembly (clean input set). */
/* Clears the u16 D_800AF7CE and calls func_80016224(0, 0). */
#include "psx_types.h"

extern u16 D_800AF7CE;
extern s32 func_80016224(s32 a, s32 b);

/* Returns retail's leftover v0: func_80016224's result. */
s32 func_8017EB28(void)
{
    D_800AF7CE = 0;
    return func_80016224(0, 0);
}
