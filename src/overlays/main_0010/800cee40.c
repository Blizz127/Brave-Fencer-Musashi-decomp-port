/* Owner re-derivation from retail assembly (clean input set). */
/* Calls entry D_800B99F6 of the two-entry handler table D_800D3480 when the index is below 2. */
#include "psx_types.h"

extern u16 D_800B99F6;
extern u32 D_800D3480[];

void func_800CEE40(void)
{
    if (D_800B99F6 < 2) {
        ((void (*)(void))D_800D3480[D_800B99F6])();
    }
}
