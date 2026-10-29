/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern u32 D_80126B78; /* guest pointer */
extern void func_801485B8(s32 base, s32 a, s32 b);

/* Forwards its two args to func_801485B8 with the pointer at +0x20 of D_80126B78's target, minus 8. Retail int function, no return value. */
s32 func_80148534(s32 a, s32 b)
{
    func_801485B8(*(s32 *)(D_80126B78 + 0x20) - 8, a, b);
#ifndef __mips__
    return 0;
#endif
}
