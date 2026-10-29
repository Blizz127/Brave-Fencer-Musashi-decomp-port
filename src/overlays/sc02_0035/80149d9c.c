/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

s32 func_80149E94(u8 *o);
s32 func_80149DD8(u8 *o);

s32 func_80149D9C(u8 *o) {
    s32 r;
    if (func_80149E94(o) != 0) {
        r = 1;
    } else {
        r = func_80149DD8(o) != 0;
    }
    return r;
}
