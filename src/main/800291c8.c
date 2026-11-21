/* Store a little-endian halfword at a byte offset in the script variables. */
#include "psx_types.h"

extern u8 D_800BA1B8[];

void func_800291C8(s32 index, s16 value)
{
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
    *(s16 *)(D_800BA1B8 + index) = value;
#else
    u8 *field = D_800BA1B8 + index;
    field[0] = (u8)value;
    field[1] = (u8)((u16)value >> 8);
#endif
}
