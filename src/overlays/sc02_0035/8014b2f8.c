/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern s32 D_80078E94;
extern u32 D_80078ECC;

void func_8014B2F8(void) {
    D_80078E94 = 0;
    (*(s32 *)&D_80078ECC) = 0;
}
