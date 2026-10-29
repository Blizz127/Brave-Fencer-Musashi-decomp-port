/* Owner re-derivation from retail assembly and m2c-assisted drafting. */
#include "psx_types.h"

extern u8 D_800AE648[];
void func_80029124(u32 bit, u8 set) {
 u32 index = bit >> 3;
 s32 mask = 1 << (bit & 7);
 if (set) D_800AE648[index] |= mask;
 else D_800AE648[index] &= ~mask;
}
