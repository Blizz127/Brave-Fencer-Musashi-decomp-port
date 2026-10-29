/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern s32 D_80078E8C;

/* D_80078E8C += n, clamped to 99999 (unsigned compare). */
void func_8014ADA8(s32 unused, s32 n)
{
    D_80078E8C += n;
    if ((u32)D_80078E8C > 99999) {
        D_80078E8C = 99999;
    }
}
