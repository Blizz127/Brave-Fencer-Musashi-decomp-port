/* Owner re-derivation from retail assembly (clean input set). */
/* Guards a sound-restart with two flag bytes, then toggles a counter against a parity flag.
 * abi s32 (table-dispatch rule): reached only via the jalr table entry at main_0012 8017F778,
 * no direct jal callers found in any region or asm/main.s; default s32 per the dispatch-table
 * ABI rule rather than asserting void from the byte match alone. */
#include "psx_types.h"

extern u8 D_800B9A15;
extern u8 D_800B9A16;
extern u16 D_800B9A02;
extern u16 D_80115114;
extern u16 D_80115112;
extern void func_800D2624(void);

s32 func_801417F8(void)
{
    if (D_800B9A15 != 0 && D_800B9A16 == 0) {
        return;
    }
    if (D_80115114 == (D_800B9A02 ^ 1)) {
        func_800D2624();
    } else {
        D_80115112++;
    }
#ifndef __mips__
    return 0;
#endif
}
