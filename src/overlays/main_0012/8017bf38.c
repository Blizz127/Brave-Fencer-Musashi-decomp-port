/* Owner re-derivation from retail assembly (clean input set). */
/* Runs func_8017C008 with both arguments 0, then increments the u16 at D_801853F0. */
#include "psx_types.h"

extern s16 D_801853F0;
extern void func_8017C008(u16 a, u16 b);

/* Returns retail's leftover v0: the incremented counter before the halfword store (untruncated). */
s32 func_8017BF38(void)
{
    s32 t;
    func_8017C008(0, 0);
    t = *(u16 *)&D_801853F0 + 1;
    *(u16 *)&D_801853F0 = t;
    return t;
}
