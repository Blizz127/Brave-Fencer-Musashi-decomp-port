/* Owner re-derivation from retail assembly (clean input set). */
/* func_80147B5C [80147B5C,80147C30) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

/* callers read v0: no (callers 80147AF8/80147B3C ignore it; retail sets none); void.
 * Each scratch word is stored in both arms of an if/else: jump2 cross-jumps the identical
 * stores to the join, and the join label keeps the reloads below after the last store. */
extern s32 D_80127090;
extern s32 D_80127094;
extern s32 D_80127098;

void func_80147B5C(u8 *o, s32 *p)
{
    if (*(u32 *)(o + 0x44) & 0x400000) {
        D_80127090 = *(s32 *)(o + 0x24);
    } else {
        D_80127090 = p[0];
    }
    if (*(u32 *)(o + 0x44) & 0x800000) {
        D_80127094 = *(s32 *)(o + 0x28);
    } else {
        D_80127094 = p[1];
    }
    if (*(u32 *)(o + 0x44) & 0x1000000) {
        D_80127098 = *(s32 *)(o + 0x2C);
    } else {
        D_80127098 = p[2];
    }
    *(s32 *)(o + 0x10) = D_80127090;
    *(s32 *)(o + 0x14) = D_80127094;
    *(s32 *)(o + 0x18) = D_80127098;
    D_80127090 = 0;
    D_80127094 = 0;
    D_80127098 = 0;
}
