#include "psx_types.h"

#include <assert.h>
#include <string.h>

s32 func_800D2CA8(s32 value, s32 shift);
void func_800D2D10(s32 packed, s32 digits, u8 *out, s32 padding);
u8 *D_800D38D8[16];
static u8 glyphs[16][2];

static void expect(u32 packed, s32 width, s32 padding, const u8 *indices, unsigned count)
{
    u8 buffer[40];
    memset(buffer, 0x6d, sizeof buffer);
    func_800D2D10((s32)packed, width, buffer + 4, padding);
    for (unsigned i = 0; i < sizeof buffer; ++i) {
        if (i < 4 || i >= 4 + 2 * count) assert(buffer[i] == 0x6d);
        else assert(buffer[i] == glyphs[indices[(i - 4) / 2]][(i - 4) % 2]);
    }
}

/* Cover every numeral in every input position. Output beyond the eight
 * packed nibbles must use glyph zero, including after a nonzero last nibble. */
static void check_wide_output(void)
{
    for (unsigned seed = 0; seed < 16; ++seed) {
        u32 packed = 0;
        u8 expected[10] = {0};
        for (unsigned column = 0; column < 8; ++column) {
            expected[column] = (u8)((seed + 5 * column) % 16);
            packed = packed * 16 + expected[column];
        }
        for (unsigned width = 8; width <= sizeof expected; ++width)
            expect(packed, (s32)width, 0, expected, width);
    }
}

int main(void)
{
    for (unsigned i = 0; i < 16; ++i) {
        D_800D38D8[i] = glyphs[i];
        glyphs[i][0] = (u8)(20 + i);
        glyphs[i][1] = (u8)(91 - i);
    }
    assert((u32)func_800D2CA8(0, 28) == 0);
    assert((u32)func_800D2CA8(16, 28) == 0x60000000);
    assert((u32)func_800D2CA8(12345678, 32) == 0x12345678);
    assert((u32)func_800D2CA8(1000000000, 0) == 0x10);
    assert((u32)func_800D2CA8(-1, -31) == 0xfffffffeu);
    expect(0x00102000, 6, 1, (const u8[]){10, 10, 1, 0, 2, 0}, 6);
    expect(0, 0x10003, 0xffff, (const u8[]){10, 10, 0}, 3);
    expect(0, 3, 1, (const u8[]){10, 10, 10}, 3);
    expect(0xfa000000, 3, 0, (const u8[]){15, 10, 0}, 3);
    check_wide_output();
    expect(0, 0xffff, -1, NULL, 0);
    expect((u32)func_800D2CA8(255, 28), 1, 0, (const u8[]){5}, 1);

    /* A later digit may read the pair written by a preceding digit. */
    u8 pairs[6] = {0};
    D_800D38D8[2] = pairs;
    func_800D2D10(0x12000000, 2, pairs, 0);
    assert(!memcmp(pairs, glyphs[1], 2) && !memcmp(pairs + 2, glyphs[1], 2));
    return 0;
}
