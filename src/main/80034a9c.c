/* Owner re-derivation from retail assembly and m2c-assisted drafting. */
#include "psx_types.h"
#ifndef NULL
#define NULL ((void *)0)
#endif

extern u8 D_800A4F17[];

void func_80034A9C(void *arg0) {
    u8 temp_a0;

    if ((*(u16 *) ((s8 *) (arg0) + (0x30))) != 0x3FFF) {
        temp_a0 = D_800A4F17[0];
        D_800A4F17[0] = 1;
        (*(s8 *) ((s8 *) (arg0) + (0x32))) = 1;
        (*(s16 *) ((s8 *) (arg0) + (0x2E))) = 0x3FF;
        (*(u16 *) ((s8 *) (arg0) + (0x30))) = 0x3FFFU;
        D_800A4F17[0] = temp_a0;
    }
}
