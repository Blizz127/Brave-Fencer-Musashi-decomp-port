/* Owner re-derivation from retail assembly (clean input set). */
/* Clears five global words. */
#include "psx_types.h"

extern s32 D_800760AC;
extern s32 D_8007609C;
extern s32 D_800760A0;
extern s32 D_800760A4;
extern s32 D_800760A8;

void func_8002AF18(void) {
    D_800760AC = 0;
    D_8007609C = 0;
    D_800760A0 = 0;
    D_800760A4 = 0;
    D_800760A8 = 0;
}
