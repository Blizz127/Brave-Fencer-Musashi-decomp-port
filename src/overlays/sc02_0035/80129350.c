/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

/* Returns retail's leftover v0: b[3], the last word copied (the sw sits in the jr delay slot). */
s32 func_80129350(s32 *a, s32 *b) {
    s32 r;
    a[1] = b[1];
    a[2] = b[2];
    r = b[3];
    a[3] = r;
    return r;
}
