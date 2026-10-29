/* Owner re-derivation from retail assembly (clean input set). */
/* Passes both arguments straight through to func_800D2EF8 (the retail words leave a0/a1 untouched before the call). */
#include "psx_types.h"

extern void func_800D2EF8(u32 a0, u32 a1);

void func_800D2C88(u32 a0, u32 a1)
{
    func_800D2EF8(a0, a1);
}
