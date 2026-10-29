/* Owner re-derivation from retail assembly and m2c-assisted drafting. */
#include "psx_types.h"
#ifndef NULL
#define NULL ((void *)0)
#endif
extern u8 func_800301A4[];
s32 func_80037CD8(u8 *);                            /* extern */
extern u8 D_8006AEF4;
extern u16 D_800A46CC[];

s32 func_80030470(void) {
    if (func_80037CD8(func_800301A4) != 0) {
        D_800A46CC[0] += 1;
        D_8006AEF4 |= 2;
    }
    return 0;
}
