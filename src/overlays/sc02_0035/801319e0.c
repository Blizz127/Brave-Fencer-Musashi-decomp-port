/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"
extern s32 func_80131CA8(char *o, s32 a);
extern void func_8002A04C(char *o);
extern s32 func_8012C218(char *o);
/* Returns retail's leftover v0: func_80131CA8's result when it is nonzero, else func_8012C218's result. */
s32 func_801319E0(char *o)
{
    s32 r;
    r = func_80131CA8(o, 0x11);
    if (r == 0) {
        if (*(u32 *)(o + 0x78)) {
            func_8002A04C(o);
        }
        r = func_8012C218(o);
    }
    return r;
}
