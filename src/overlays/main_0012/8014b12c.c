/* Owner re-derivation from retail assembly (clean input set). */
/* Tests the 0x80 flag of the fixed object D_80126B58. */
#include "psx_types.h"

extern u8 D_80126B58[];
extern u32 func_8014B154(void *obj);

u32 func_8014B12C(void)
{
    return func_8014B154(D_80126B58);
}
