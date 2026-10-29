/* Owner re-derivation from retail assembly (clean input set). */
/* Clears two global words. */
#include "psx_types.h"

extern s32 D_800AE6F4;
extern s32 D_800AE70C;

void func_8001A0FC(void) {
    D_800AE6F4 = 0;
    D_800AE70C = 0;
}
