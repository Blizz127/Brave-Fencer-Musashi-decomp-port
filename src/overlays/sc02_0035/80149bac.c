/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"
extern s32 func_8015DE24(char *o);
extern s32 func_80157510(char *o);
/* Returns retail's leftover v0: func_8015DE24's or func_80157510's result. */
s32 func_80149BAC(char *o)
{
    s32 r;
    if (*(u32 *)(o + 0x178) != 0) {
        r = func_8015DE24(o);
    } else {
        *(u8 *)(o + 0x1AA) = 1;
        r = func_80157510(o);
    }
    return r;
}
