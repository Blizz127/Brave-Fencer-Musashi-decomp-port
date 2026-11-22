/* Pack the low decimal digits and shift them into the text writer's input. */
#include "psx_types.h"

#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
#define DIGIT_SHIFT(n) (n)
#else
#define DIGIT_SHIFT(n) ((u32)(n) & 31u)
#endif

s32 func_800D2CA8(s32 value, s32 shift)
{
    u32 packed = 0;
    s32 remaining = value;
    s32 bit = 0;

    while (remaining >= 10) {
        value /= 10;
        packed |= (u32)(remaining - ((value << 1) + (value << 3))) << DIGIT_SHIFT(bit);
        remaining = value;
        bit += 4;
    }
    packed |= (u32)remaining << DIGIT_SHIFT(bit);
    return (s32)(packed << DIGIT_SHIFT(shift));
}
