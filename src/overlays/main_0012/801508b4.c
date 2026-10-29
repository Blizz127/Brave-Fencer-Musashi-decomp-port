/* Owner re-derivation from retail assembly (clean input set). */
/* If func_8014A2E4 yields nonzero, passes it to func_8014A380 and returns 1; else returns 0. */
#include "psx_types.h"

extern s32 func_801508B4(void *obj)
{
    s32 r = func_8014A2E4(obj);

    if (r != 0) {
        func_8014A380(obj, r);
        return 1;
    } else {
        return 0;
    }
}
