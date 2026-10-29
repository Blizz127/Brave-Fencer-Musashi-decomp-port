/* Owner re-derivation from retail assembly and m2c-assisted drafting. */
#include "psx_types.h"
#ifndef NULL
#define NULL ((void *)0)
#endif

s32 func_8003D650(s32, s32, s32);   /* extern */
s32 func_8003EDE8(s32, s16, s16);           /* extern */
extern s16 D_800760E4;

s16 func_8002D858(void) {
    D_800760E4 = 0x64;
    func_8003D650(0, 1, 0);
    func_8003EDE8(0, D_800760E4, D_800760E4);
    return D_800760E4;
}
