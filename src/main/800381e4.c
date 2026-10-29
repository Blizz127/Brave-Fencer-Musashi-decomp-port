/* Owner re-derivation from retail assembly and m2c-assisted drafting. */
#include "psx_types.h"
#ifndef NULL
#define NULL ((void *)0)
#endif
extern u8 D_800B9CF0[];
u8 func_800381E4(s32 arg0, s32 arg1) {
 arg1 = (s32)(arg1 << 16) >> 13;
 arg1 += arg0 * 0x1fc;
 return D_800B9CF0[arg1];
}
