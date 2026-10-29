/* Owner re-derivation from retail assembly (clean input set). */
/* Same as func_8017CFFC with the block D_80182688. */
#include "psx_types.h"

extern u8 D_80182688[];
extern void func_8016EE40(u8 *p, s32 v);

void func_8017CFD4(void)
{
    func_8016EE40(D_80182688, 0x1000000);
}
