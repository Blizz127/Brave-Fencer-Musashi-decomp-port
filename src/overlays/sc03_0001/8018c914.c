/* Return 1 for insufficient funds, or commit the adjusted balance and return 0. */
#include "psx_types.h"

extern s32 D_80078E8C;

s32 func_8018C914(s32 delta)
{
    u32 bits = (u32)D_80078E8C + (u32)delta;
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
    /* The PS1 compiler reinterprets a wrapped word as two's complement. */
    s32 balance = (s32)bits;
#else
    s32 balance = (bits & 0x80000000u) ? -1 - (s32)~bits : (s32)bits;
#endif

    if (balance > 99999)
        balance = 99999;
    if (balance < 0)
        return 1;
    D_80078E8C = balance;
    return 0;
}
