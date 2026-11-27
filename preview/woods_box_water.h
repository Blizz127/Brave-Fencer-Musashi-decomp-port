/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_BOX_WATER_H
#define BFM_WOODS_BOX_WATER_H
#include "bedroom_field.h"

typedef struct {
    const uint8_t *colors;
    unsigned x[2], y[2];
} BfmWoodsBoxWater;

int bfm_woods_box_water_load(BfmWoodsBoxWater *water, const uint8_t *scene, size_t bytes);
void bfm_woods_box_water_draw(const BfmWoodsBoxWater *water, unsigned age,
                              BfmBedroomField *field, uint16_t tiles[512][1024],
                              uint32_t *pixels, void (*draw_to)(void *, int), void *user);

#endif
