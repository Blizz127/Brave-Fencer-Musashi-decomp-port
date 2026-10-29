/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"
extern u8 *func_8012C1B8(char *o);
extern s32 func_8012CAE4(char *o);
extern void func_8001C214(u8 *p, s32 a);

/* Returns retail's leftover v0: func_8012CAE4's result, else the constant 1 stored to +2. */
s32 func_80144A98(char *o)
{
    u8 *p = func_8012C1B8(o);
    s32 r;
    *(u8 **)(o + 0x20) = p;
    if (p == 0) {
        r = func_8012CAE4(o);
    } else {
        func_8001C214(p, 0);
        r = 1;
        *(s16 *)(o + 2) = r;
    }
    return r;
}
