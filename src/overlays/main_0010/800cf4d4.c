/* Owner re-derivation from retail assembly (clean input set). */
/* Calls entry D_800B99FC of the handler table D_800D3490. */
#include "psx_types.h"

extern u16 D_800B99FC;
extern u32 D_800D3490[];

void func_800CF4D4(void)
{
    ((void (*)(void))D_800D3490[D_800B99FC])();
}
