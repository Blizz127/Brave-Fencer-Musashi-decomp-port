/* Owner re-derivation from retail assembly and m2c-assisted drafting. */
#include "psx_types.h"
#ifndef NULL
#define NULL ((void *)0)
#endif

s32 func_80029344();                            /* extern */
extern s32 D_80078E90;

void func_80029664(void) {
    D_80078E90 = 0x528;
    func_80029344();
}
