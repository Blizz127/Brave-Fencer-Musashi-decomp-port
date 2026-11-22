#include "psx_types.h"

/* The matching compiler emits R3000 shifts, which mask the count in hardware. */
#if defined(__mips__) && defined(__LITTLE_ENDIAN__) && !defined(__LP64__) && defined(__GNUC__) && __GNUC__ == 2
#define SHIFT_COUNT(count) (count)
#else
#define SHIFT_COUNT(count) ((u32)(count) & 31u)
#endif

/* Pack decimal digits into nibbles, starting with the units digit. */
u32 func_8017DC48(s32 value, s32 shift)
{
    u32 packed = 0;
    s32 part = value;
    u32 bit = 0;

    while (part >= 10) {
        value /= 10;
        packed |= (u32)(part - ((value << 1) + (value << 3))) << SHIFT_COUNT(bit);
        part = value;
        bit += 4;
    }
    packed |= (u32)part << SHIFT_COUNT(bit);
    return packed << SHIFT_COUNT(shift);
}
