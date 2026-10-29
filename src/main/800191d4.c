/* Owner re-derivation from retail assembly (clean input set). */
/* Returns 1 unless the current id is valid, the check call fails and it equals arg. */
#include "psx_types.h"

extern s32 D_80078E30;
extern s32 func_8001923C(s32 arg0);

s32 func_800191D4(s32 arg0)
{
    s32 *cur = &D_80078E30;

    if (*cur >= 0) {
        goto check;
    }
    return 1;
check:
    if (func_8001923C(arg0) == 0) {
        return *cur != arg0;
    }
    return 1;
}
