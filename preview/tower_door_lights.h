/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_DOOR_LIGHTS_H
#define BFM_TOWER_DOOR_LIGHTS_H

#include "forest_dust.h"

enum { BFM_TOWER_DOOR_CARDS = 14 };

typedef struct {
    int16_t rim[12][3];
    int16_t pivot_x, pivot_y;
    uint8_t width, height, u, v;
} BfmTowerDoorLights;

int bfm_tower_door_lights_load(BfmTowerDoorLights *lights, const uint8_t *scene, size_t bytes);
size_t bfm_tower_door_light_cards(const BfmTowerDoorLights *lights, const BfmForestCamera *camera,
                                 BfmForestDustCard out[BFM_TOWER_DOOR_CARDS]);

#endif
