/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_STREET_SCENERY_H
#define BFM_VILLAGE_STREET_SCENERY_H

#include "bedroom_field.h"

typedef struct BfmVillageStreetScenery BfmVillageStreetScenery;

BfmVillageStreetScenery *bfm_village_street_scenery_load(const uint8_t *scene,
                                                        size_t bytes, uint32_t room);
void bfm_village_street_scenery_free(BfmVillageStreetScenery *scenery);
int bfm_village_street_scenery_prepare(BfmVillageStreetScenery *scenery,
                                      const BfmBedroomView *view,
                                      const uint16_t tiles[512][1024], uint32_t *pixels);
int bfm_village_street_scenery_next_depth(const BfmVillageStreetScenery *scenery);
void bfm_village_street_scenery_draw_to(BfmVillageStreetScenery *scenery, int depth);

#endif
