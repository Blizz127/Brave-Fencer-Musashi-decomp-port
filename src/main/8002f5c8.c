/* Owner re-derivation from retail assembly and m2c-assisted drafting. */
#include "psx_types.h"
#ifndef NULL
#define NULL ((void *)0)
#endif

s32 func_8002F620();                            /* extern */
extern u16 *D_800A4EE8;
extern s16 D_800A4EF0[];

void func_8002F5C8(u16 *arg0) {
    if (D_800A4EF0[0] != 0) {
        func_8002F620();
    }
    D_800A4EF0[0] = (s16) *arg0;
    D_800A4EE8 = arg0;
}
