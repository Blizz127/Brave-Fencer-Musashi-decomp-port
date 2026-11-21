/* Fresh isolated derivation from sc01_0000 retail instructions. */
#include "psx_types.h"
s32 func_801488A8(u8 *object)
{
    u16 value;
    u32 low;
    switch (object[0xA9]) {
    case 'A':
        return *(u16 *)(object + 0xAA) & 0xF000;
    case 'S':
    case 's':
        value = *(u16 *)(object + 0xAE);
        low = value & 0xFF;
        if (low != 0x80) goto nondefault;
        if ((value >> 8) != low) goto nondefault;
        return 0;
    default:
        return 0;
    }
nondefault:
    return 1;
}
