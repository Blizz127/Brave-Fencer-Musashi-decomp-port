/* Own retail: this predicate deliberately mixes a bitmask and Boolean results. */
#include "psx_types.h"
s32 func_80148980(u8 *o)
{
    switch (o[0xA9]) {
    case 0x41:
        return *(u16 *)(o + 0xAA) & 0x4000;
    case 0x53:
    case 0x73:
        return (*(u16 *)(o + 0xAE) >> 8) > 0xC0;
    default:
        return 0;
    }
}
