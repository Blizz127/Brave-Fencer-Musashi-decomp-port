/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_PRESS_EFFECT_H
#define BFM_TOWER_PRESS_EFFECT_H

#include "forest_dust.h"

enum { BFM_TOWER_PRESS_CARDS = 6 };
typedef struct {
    uint32_t rim[BFM_TOWER_PRESS_CARDS + 1];
} BfmTowerPressColors;

int bfm_tower_press_colors_load(BfmTowerPressColors *colors,
                                const uint8_t *scene, size_t size);
size_t bfm_tower_press_cards(const int16_t center[3], unsigned remaining,
                             const BfmForestCamera *camera, const BfmTowerPressColors *colors,
                             BfmForestDustCard out[BFM_TOWER_PRESS_CARDS]);
#endif
