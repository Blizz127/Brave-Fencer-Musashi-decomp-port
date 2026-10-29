/* Owner re-derivation from retail assembly (clean input set). UNVERIFIED DRAFT: not compiled yet. */
/* Hands the module descriptor block D_800CBB88 to func_800183E0. */
#include "psx_types.h"

extern u8 D_800CBB88[];
extern int func_800183E0(unsigned int value);

void func_800CAE0C(void)
{
    func_800183E0((u32)D_800CBB88);
}
