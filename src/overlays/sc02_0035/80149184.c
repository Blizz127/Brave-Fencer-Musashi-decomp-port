/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"
extern s32 func_801491C4(char *o);

s32 func_80149184(char *o)
{
    s32 v = func_801491C4(o);

    return (*(u16 *)(*(u32 *)(o + 0x20) + 0x12) - v) & 0xFFF;
}
