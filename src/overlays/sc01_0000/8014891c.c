/* Fresh isolated derivation from sc01_0000 retail instructions. */
#include "psx_types.h"
s32 func_8014891C(u8 *object)
{
    s32 result;
    switch (object[0xA9]) {
    case 'A': result = *(u16 *)(object + 0xAA) & 0x1000; break;
    case 'S':
    case 's': result = (*(u16 *)(object + 0xAE) >> 8) < 0x40; break;
    default: result = 0; break;
    }
    return result;
}
