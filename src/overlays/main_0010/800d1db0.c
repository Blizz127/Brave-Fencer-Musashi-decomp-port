/* Owner re-derivation from retail assembly (clean input set). */
/* Classifies the pending mode D_80078E50 against the current mode word D_800B9A08: returns 1 only when the current mode is not 0x3005, not classified by func_800CFC5C, the pending mode passes func_800CFD68 and is not classified; else 2. */
#include "psx_types.h"

extern s32 func_800CFC5C(s32 id);
extern s32 func_800CFD68(s32 id);
extern s16 D_800B9A08;
extern u16 D_80078E50;

s32 func_800D1DB0(void)
{
    s32 r;

    if (D_800B9A08 == 0x3005) {
        return 2;
    }
    if (func_800CFC5C(D_800B9A08) != 0) {
        return 2;
    }
    if (func_800CFD68(D_80078E50) == 0) {
        return 2;
    }
    r = func_800CFC5C(D_80078E50);
    if (r == 0) {
        return 1;
    }
    return 2;
}
