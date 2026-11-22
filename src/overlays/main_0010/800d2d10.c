/* Write each selected glyph pair from the current numeral table into the text. */
#include "psx_types.h"

extern u8 *D_800D38D8[];

/* The original compiler keeps the source nibble live beside the glyph index. */
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
#define NUMERAL_NIBBLE_REG __asm__("$8")
#else
#define NUMERAL_NIBBLE_REG
#endif

void func_800D2D10(s32 value, s32 digits, u8 *output, s32 padding)
{
    u32 packed = (u32)value;
    s16 i;

    for (i = 0; i < (s16)digits; ++i) {
        register u32 digit NUMERAL_NIBBLE_REG = packed >> 28;
        s16 glyph = digit;
        if ((s16)padding) {
            if (digit) {
                padding = 0;
            } else {
                glyph = 10;
                if ((s16)padding < 0) glyph = i == (s16)digits - 1 ? 0 : 10;
            }
        }
        packed <<= 4;
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
        *(u16 *)output = *(const u16 *)D_800D38D8[glyph];
#else
        {
            u8 *pair = D_800D38D8[glyph];
            u8 lo = pair[0];
            u8 hi = pair[1];
            output[0] = lo;
            output[1] = hi;
        }
#endif
        output += 2;
    }
}
