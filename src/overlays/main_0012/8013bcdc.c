/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern u32 D_8017EAD4; /* guest function pointer */

void func_8013BCDC(void) {
    s32 unused_frame[4];

    if (D_8017EAD4 != 0) {
        ((void (*)(void))D_8017EAD4)();
    }
}
