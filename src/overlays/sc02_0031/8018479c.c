/* Owner re-derivation from retail assembly (clean input set). */
/* abi s32: no jal callers found; body returns v0=2 (not falling off the
 * end), so the value is kept as the return rather than asserted void. */
#include "psx_types.h"

s32 func_8018479C(u8 *o)
{
    u8 *v1 = *(u8 **)(o + 0x6C);
    s32 v0 = 2;
    *(s16 *)(v1 + 0xFE) = v0;
    return v0;
}
