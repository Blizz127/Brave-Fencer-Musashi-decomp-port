/* Owner re-derivation from retail assembly (clean input set). */
/* Runs the handler selected by the u16 state D_800B99F0 from the table D_800D3430 (states 0..0x13). Returns retail's leftover v0: the handler's result, or the out-of-range compare flag (0). */
#include "psx_types.h"

extern u16 D_800B99F0;
extern u32 D_800D3430[];

s32 func_800CEDFC(void)
{
    s32 r;

    r = D_800B99F0 < 0x14;
    if (r) {
        r = ((s32 (*)(void))D_800D3430[D_800B99F0])();
    }
    return r;
}
