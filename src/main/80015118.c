/* Owner re-derivation from retail assembly and m2c-assisted drafting. */
#include "psx_types.h"
#ifndef NULL
#define NULL ((void *)0)
#endif

void func_80015118(u16 *arg0) {
    if ((u32) (*arg0 - 0x15) < 0xD7U) {
        *arg0 = 0x80;
    }
}
