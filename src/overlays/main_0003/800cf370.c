/* Owner re-derivation from retail assembly and m2c-assisted drafting. */
#include "psx_types.h"
#ifndef NULL
#define NULL ((void *)0)
#endif

s32 func_800118AC();                            /* extern */
s32 func_800146B0(s32);                     /* extern */
extern s32 D_800B9A18;

void func_800CF370(void) {
    func_800146B0(1);
    D_800B9A18 = 2;
    func_800118AC();
}
