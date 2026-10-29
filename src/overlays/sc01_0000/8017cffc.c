/* Owner re-derivation from retail assembly (clean input set). */
/* Starts the fade-in by calling func_8016EE40 with the block D_80182694 and the constant 0x1000000. */
#include "psx_types.h"

extern u8 D_80182694[];
extern void func_8016EE40(u8 *p, s32 v);

void func_8017CFFC(void)
{
    func_8016EE40(D_80182694, 0x1000000);
}
