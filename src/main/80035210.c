/* Owner re-derivation from retail assembly and m2c-assisted drafting. */
#include "psx_types.h"
#ifndef NULL
#define NULL ((void *)0)
#endif

s32 func_8003C438(s32, s32);                    /* extern */
s32 func_8003C498(s32);                         /* extern */
extern s32 D_80076218;

s32 func_80035210(s32 arg0, s32 arg1) {
    s32 result;
    func_8003C498(D_80076218);
    result = func_8003C438(arg0, arg1);
    D_80076218 += arg1;
    return result;
}
