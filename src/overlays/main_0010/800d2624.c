/* Owner re-derivation from retail assembly (clean input set). */
/* Clears the flag D_800B9A13 and sends sound command 0x15 with argument 0. */
#include "psx_types.h"

extern void func_8002D4C8(s32 a, s32 b);
extern u8 D_800B9A13;

void func_800D2624(void)
{
    D_800B9A13 = 0;
    func_8002D4C8(0x15, 0);
}
