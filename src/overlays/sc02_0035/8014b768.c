/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern s32 D_80078E9C;

void func_8014B768(s32 a0, s32 a1) {
    D_80078E9C -= a1;
    if (*(s16 *)((u8 *)&D_80078E9C + 2) < 0) {
        D_80078E9C = 0;
    }
}
