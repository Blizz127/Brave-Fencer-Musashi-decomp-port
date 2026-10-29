/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"
extern s32 func_80147364(s32 a, s32 b);
extern s32 D_8012707C;
/* Returns retail's leftover v0: the old D_8012707C (equal to a) when it matched, else func_80147364's result. */
s32 func_80147324(u16 a)
{
    s32 r;
    r = D_8012707C;
    if (a == ((u32)r)) {
        D_8012707C = 0;
    } else {
        r = func_80147364(a, 0);
    }
    return r;
}
