/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern u16 D_80126B5E, D_80126B62, D_80126B66;
extern u16 D_80126940;
extern s16 D_80126944;
extern s16 D_80126942;

/* Returns retail's leftover v0: the u16 D_80126B5E (zero-extended). */
s32 func_8012A048(u8 *o, u32 a, u8 b) {
    s32 r;
    *(u32 *)o = a;
    o[4] = b;
    *(u32 *)(o + 0x9C) = 0;
    *(u16 *)(o + 0xA0) = 0;
    *(u16 *)(o + 0xA2) = 0;
    r = D_80126B5E;
    D_80126940 = r;
    (*(u16 *)&D_80126942) = D_80126B62;
    (*(u16 *)&D_80126944) = D_80126B66;
    return r;
}
