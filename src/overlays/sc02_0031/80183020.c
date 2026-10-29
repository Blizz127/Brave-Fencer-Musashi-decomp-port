/* Owner re-derivation from retail assembly (clean input set). */
/* abi s32 RETURN_NO_VALUE: no jal callers found; tail-calls the void
 * func_801832D0(a, b) and falls off the end without touching v0. */
#include "psx_types.h"

void func_801832D0(u32 a, u32 b);

s32 func_80183020(void)
{
    func_801832D0((u32)-0x380, 0);
#ifndef __mips__
    return 0;
#endif
}
