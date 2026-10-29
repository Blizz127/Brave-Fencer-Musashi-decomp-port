/* Owner re-derivation from retail assembly (clean input set). */
/* Same shape as the sibling func_801626B8: seeds a masked RNG draw through
 * func_800D0E30, then treats the result's two bytes as a 0-99 "ones" and a
 * "hundreds" digit (hi*100 + lo). No caller found in any region. */
#include "psx_types.h"

extern s32 func_800291DC(s32);
extern s32 func_800D0E30(u16 *);

s16 func_801620C4(void) {
    u16 w;
    s32 hi;
    s32 lo;
    w = func_800291DC(0x8) & 0x7FFF;
    w = func_800D0E30(&w);
    hi = ((u8 *)&w)[1];
    lo = ((u8 *)&w)[0];
    return hi * 100 + lo;
}
