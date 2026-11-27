/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_WATER_EFFECT_H
#define BFM_WOODS_WATER_EFFECT_H

#include "bedroom_figure.h"
#include "forest_water_effect.h"

/* Initialize from the mounted Cleft Woods room. Use the shared effect's begin
 * and tick once per ordinary field update, independently of the draw rate. */
int bfm_woods_water_effect_load(BfmForestWaterEffect *effect,
                                const uint8_t *scene, size_t bytes);

/* Submit to the prepared player/visitor queue with the active field camera. */
int bfm_woods_water_effect_append(const BfmForestWaterEffect *effect,
                                  BfmBedroomFigure *figure, const BfmBedroomView *view);

#endif
