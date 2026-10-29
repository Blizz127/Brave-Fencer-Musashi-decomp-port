/* Owner re-derivation from retail assembly (clean input set). */
/* abi s32: no jal callers found; the function computes and returns a value, a0
 * is clobbered before use so it takes no meaningful parameter. */
#include "psx_types.h"

extern u8 D_80078EAF;

s32 func_801832F4(void)
{
    if (D_80078EAF == 4) {
        return 0x3C;
    }
    return 0xA;
}
