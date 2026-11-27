/* SPDX-License-Identifier: MIT */
#include "woods_box_water.h"
#include <string.h>

enum { SHORE_COLORS = 32, FALL_COLORS = 16 };
static const unsigned widths[2] = {SHORE_COLORS, FALL_COLORS};

static uint16_t little16(const uint8_t *p)
{
    return (uint16_t)(p[0] | (unsigned)p[1] << 8);
}

int bfm_woods_box_water_load(BfmWoodsBoxWater *water, const uint8_t *scene, size_t bytes)
{
    /* SC02 member 11 packs both color cycles just before their upload records. */
    const size_t colors = UINT32_C(0x8019611c) - UINT32_C(0x80128158);
    const size_t uploads = colors + 2 * (SHORE_COLORS + FALL_COLORS);
    if (!water || !scene || bytes < uploads + 32) return 0;
    BfmWoodsBoxWater mounted = {.colors = scene + colors};
    for (unsigned i = 0; i < 2; ++i) {
        const uint8_t *record = scene + uploads + 16 * i;
        unsigned x = little16(record + 4), y = little16(record + 6);
        if (record[0] != 9 || little16(record + 8) != widths[i] ||
            little16(record + 10) != 1 || x > 1024 - widths[i] || y >= 512)
            return 0;
        mounted.x[i] = x;
        mounted.y[i] = y;
    }
    *water = mounted;
    return 1;
}

void bfm_woods_box_water_draw(const BfmWoodsBoxWater *water, unsigned age,
                              BfmBedroomField *field, uint16_t tiles[512][1024],
                              uint32_t *pixels, void (*draw_to)(void *, int), void *user)
{
    uint16_t previous[SHORE_COLORS + FALL_COLORS];
    unsigned offset = 0;
    for (unsigned i = 0; i < 2; ++i) {
        unsigned width = widths[i];
        uint16_t *row = &tiles[water->y[i]][water->x[i]];
        memcpy(previous + offset, row, width * sizeof(*row));
        for (unsigned j = 0; j < width; ++j) {
            unsigned index = offset + (age + j) % width;
            row[j] = little16(water->colors + 2 * index) | UINT16_C(0x8000);
        }
        offset += width;
    }
    bfm_bedroom_field_draw(field, tiles, pixels, draw_to, user);
    /* The room borrows VRAM from the surrounding field scenes. */
    for (unsigned i = 2; i-- > 0;) {
        unsigned width = widths[i];
        offset -= width;
        memcpy(&tiles[water->y[i]][water->x[i]], previous + offset, width * sizeof(*previous));
    }
}
