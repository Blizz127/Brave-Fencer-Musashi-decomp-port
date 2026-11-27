/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_STREET_SCENERY_H
#define BFM_VILLAGE_STREET_SCENERY_H

#include "bedroom_figure.h"
#include "gameplay_run.h"

typedef struct BfmVillageStreetScenery BfmVillageStreetScenery;

BfmVillageStreetScenery *bfm_village_street_scenery_load(const uint8_t *scene,
                                                        size_t bytes, uint32_t room);
void bfm_village_street_scenery_free(BfmVillageStreetScenery *scenery);
/* The running scene update clock drives the church bell's swing. */
void bfm_village_street_scenery_step(BfmVillageStreetScenery *scenery,
                                     unsigned scene_tick, unsigned progress);
/* Use the current street daylight colors for the bell's three directional lights. */
void bfm_village_street_scenery_lights(BfmVillageStreetScenery *scenery, const BfmBedroomLight lights[3]);
/* The weather vane uses the field's shared RNG once a gust starts or ends. */
void bfm_village_street_vane_step(BfmVillageStreetScenery *scenery, BfmGameplayRun *run);
int bfm_village_street_vane_add(const BfmVillageStreetScenery *scenery, BfmBedroomFigure *figure,
                               const BfmBedroomView *view);
int bfm_village_street_scenery_prepare(BfmVillageStreetScenery *scenery,
                                      const BfmBedroomView *view,
                                      const uint16_t tiles[512][1024], uint32_t *pixels, unsigned draw_page);
int bfm_village_street_scenery_next_depth(const BfmVillageStreetScenery *scenery);
void bfm_village_street_scenery_draw_to(BfmVillageStreetScenery *scenery, int depth);

#endif
