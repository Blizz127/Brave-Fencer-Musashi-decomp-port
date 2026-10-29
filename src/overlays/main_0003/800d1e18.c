/* Owner re-derivation from retail assembly and m2c-assisted drafting. */
#include "psx_types.h"
#ifndef NULL
#define NULL ((void *)0)
#endif
extern s32 D_800D942C[];
extern u8 D_800EC9FC[];
s32 func_8001BFA0();                                /* extern */
s32 func_80045374(u8 *, s32);                       /* extern */

s32 func_800D1E18(void) {
    return func_80045374(D_800EC9FC, *(s32 *)(((u8 *)D_800D942C) + (func_8001BFA0() << 5))) != 0;
}
