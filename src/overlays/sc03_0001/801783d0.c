/* Pack decimal digits into the guest word before applying the caller's shift. */
#include "psx_types.h"

#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
#define DIGIT_SHIFT(value, count) ((value) << (count))
#else
static s32 digit_shift(s32 value, s32 count)
{
    u32 word = (u32)value << ((u32)count & 31u);
    return (word & 0x80000000u) ? -1 - (s32)~word : (s32)word;
}
#define DIGIT_SHIFT(value, count) digit_shift(value, count)
#endif

s32 func_801783D0(s32 value, s32 shift)
{
    s32 packed = 0;
    s32 position = 0;
    s32 current = value;
    while (current >= 10) {
        value = value / 10;
        packed |= DIGIT_SHIFT(current - ((value << 1) + (value << 3)), position);
        current = value;
        position += 4;
    }
    packed |= DIGIT_SHIFT(current, position);
    return DIGIT_SHIFT(packed, shift);
}
