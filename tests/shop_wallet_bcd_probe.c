#include <assert.h>
#include <stdint.h>
#include "psx_types.h"

s32 func_801783D0(s32 value, s32 shift);

static u32 expected(s32 value)
{
    u32 digit_position = 0;
    u32 packed = 0;
    if (value < 0)
        return (u32)value;
    do {
        packed |= (u32)(value % 10) << (digit_position & 31u);
        value /= 10;
        digit_position += 4;
    } while (value != 0);
    return packed;
}

int main(void)
{
    static const s32 boundaries[] = {-312, -1, INT32_MIN, INT32_MAX, 1000000000};
    s32 balance, shift;
    unsigned i;
    for (balance = 0; balance <= 99999; ++balance)
        assert((u32)func_801783D0(balance, 0) == expected(balance));
    for (i = 0; i < sizeof boundaries / sizeof boundaries[0]; ++i)
        for (shift = -64; shift < 96; ++shift)
            assert((u32)func_801783D0(boundaries[i], shift) ==
                   (expected(boundaries[i]) << ((u32)shift & 31u)));
    return 0;
}
