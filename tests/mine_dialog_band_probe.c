#include "mine_dialog_band.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

u8 D_801A0D8A;
u8 D_801A0CB2;
u8 D_801A0C90;
MineBandColor D_8019F950[4];

static uint32_t rng = 1221;

static u8 next_byte(void)
{
    rng = rng * UINT32_C(1664525) + UINT32_C(1013904223);
    return (u8)(rng >> 24);
}

static u8 color(unsigned intensity, unsigned channel)
{
    return (u8)((intensity * channel) / 16u);
}

static void check(s16 enabled, s16 selector)
{
    MineBandColor expected[4];
    u8 band[84], expected_band[84];
    unsigned i;
    unsigned red = D_801A0D8A = next_byte();
    unsigned green = D_801A0CB2 = next_byte();
    unsigned blue = D_801A0C90 = next_byte();

    for (i = 0; i < sizeof(band); i++)
        band[i] = next_byte();
    for (i = 0; i < sizeof(expected); i++)
        ((u8 *)expected)[i] = next_byte();
    memcpy(D_8019F950, expected, sizeof(expected));
    memcpy(expected_band, band, sizeof(band));

    if (enabled && selector >= 0 && selector < 5) {
        if (selector == 1) {
            for (i = 0; i < 4; i++) {
                unsigned r = i == 0 ? 10 : i == 2 ? 255 : 143;
                unsigned g = i == 0 ? 128 : i == 1 ? 200 : 255;
                unsigned b = i == 3 ? 180 : 255;
                expected[i].red = color(r, red);
                expected[i].green = color(g, green);
                expected[i].blue = color(b, blue);
            }
        } else if (selector == 0 || selector == 2) {
            for (i = 0; i < 2; i++) {
                MineBandColor *row = &expected[(unsigned)selector + i];
                row->red = color(band[68 + 3 * i], red);
                row->green = color(band[69 + 3 * i], green);
                row->blue = color(band[70 + 3 * i], blue);
            }
        } else {
            expected_band[32] = (u8)(red * 8);
            expected_band[33] = (u8)(green * 8);
            expected_band[34] = (u8)(blue * 8);
        }
    }
    if (!enabled || selector < 0 || selector > 4 || selector == 1)
        func_801299C8(enabled, selector, NULL);
    else
        func_801299C8(enabled, selector, band);
    assert(memcmp(expected, D_8019F950, sizeof(expected)) == 0);
    assert(memcmp(expected_band, band, sizeof(band)) == 0);
    assert(D_801A0D8A == red && D_801A0CB2 == green && D_801A0C90 == blue);
}

int main(void)
{
    unsigned i;
    _Static_assert(sizeof(MineBandColor) == 4, "color slot size");
    _Static_assert(offsetof(MineBandColor, blue) == 2, "blue channel offset");
    for (i = 0; i < 4096; i++) {
        int selector;
        for (selector = -1; selector <= 5; selector++) {
            check(0, (s16)selector);
            check(1, (s16)selector);
            check(-1, (s16)selector);
            check(256, (s16)selector);
        }
    }
    check(-32768, -32768);
    check(32767, 32767);
    return 0;
}
