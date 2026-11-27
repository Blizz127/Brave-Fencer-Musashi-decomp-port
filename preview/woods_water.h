/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_WATER_H
#define BFM_WOODS_WATER_H

#include "bedroom_field.h"

enum { BFM_WOODS_WATER_COLORS = 32 };

typedef struct {
    uint16_t colors[BFM_WOODS_WATER_COLORS];
} BfmWoodsWater;

void bfm_woods_water_load(BfmWoodsWater *water, const uint16_t tiles[512][1024]);
void bfm_woods_water_draw(const BfmWoodsWater *water, unsigned field_age,
                          BfmBedroomField *field, uint16_t tiles[512][1024],
                          uint32_t *pixels, void (*draw_to)(void *, int), void *user);

#endif
