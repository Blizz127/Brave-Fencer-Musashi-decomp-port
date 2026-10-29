/* Owner re-derivation from retail assembly and m2c-assisted drafting. */
#include "psx_types.h"
#ifndef NULL
#define NULL ((void *)0)
#endif

s32 func_80042580();                            /* extern */
s32 func_800D2D68(s32);                         /* extern */

void func_800D2AA0(s32 arg0) {
    if (arg0 == 0) {
        func_80042580();
    }
    func_800D2D68(arg0);
}
