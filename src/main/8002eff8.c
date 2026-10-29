/* Owner re-derivation from retail assembly (clean input set). */
/* Moves the given voice bits between the free mask and the reserved mask (mode 1 reserves them, otherwise releases them). */
#include "psx_types.h"

extern s32 D_800A2B98;
extern s32 D_800C7D20;

void func_8002EFF8(s32 mode, s32 bits) {
    if (mode == 1) {
        D_800A2B98 &= ~bits;
        D_800C7D20 |= bits;
    } else {
        D_800C7D20 &= ~bits;
        D_800A2B98 |= bits;
    }
}
