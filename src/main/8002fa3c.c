/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame step of a 16-tick fade: shrinks the remaining level by the step and applies it, then resets to 0 and clears the active flag when done. */
#include "psx_types.h"

extern void func_8003EDE8(s32, s16, s16);
extern s16 D_800760E4;
extern s16 D_800760E8;
extern s16 D_800760EC;
extern s32 D_800A4F24;

void func_8002FA3C(void) {
    if (++D_800760E8 < 0x10 && D_800760E4 > D_800760EC) {
        D_800760E4 -= D_800760EC;
        func_8003EDE8(0, D_800760E4, D_800760E4);
        return;
    }
    func_8003EDE8(0, 0, 0);
    D_800A4F24 = 0;
}
