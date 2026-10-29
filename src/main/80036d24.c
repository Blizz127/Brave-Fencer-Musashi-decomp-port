/* Owner re-derivation from retail assembly and m2c-assisted drafting. */
#include "psx_types.h"
#ifndef NULL
#define NULL ((void *)0)
#endif

extern s32 D_8006AEE8;
extern s32 D_80078F10;
extern s8 D_800A4F1A;

void func_80036D24(void) {
    D_80078F10 = 0;
    if (D_8006AEE8 < 2) {
        D_8006AEE8 = 0;
        D_800A4F1A = 0;
    }
}
