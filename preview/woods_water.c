/* SPDX-License-Identifier: MIT */
#include "woods_water.h"
#include <string.h>

enum { WATER_ROW = 492 };

void bfm_woods_water_load(BfmWoodsWater *water, const uint16_t tiles[512][1024])
{
    memcpy(water->colors, tiles[WATER_ROW], sizeof water->colors);
}

void bfm_woods_water_draw(const BfmWoodsWater *water, unsigned field_age,
                          BfmBedroomField *field, uint16_t tiles[512][1024],
                          uint32_t *pixels, void (*draw_to)(void *, int), void *user)
{
    /* The surface cycles the mounted room colors above the daylight-shaded
     * bed. Keep the daylight transition's working palette between fields. */
    uint16_t *row = tiles[WATER_ROW];
    uint16_t daylight[BFM_WOODS_WATER_COLORS];
    memcpy(daylight, row, sizeof daylight);
    for (unsigned i = 0; i < BFM_WOODS_WATER_COLORS; ++i)
        row[i] = water->colors[(i + field_age) % BFM_WOODS_WATER_COLORS];
    bfm_bedroom_field_draw(field, tiles, pixels, draw_to, user);
    memcpy(row, daylight, sizeof daylight);
}
