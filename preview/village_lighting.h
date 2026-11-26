/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_LIGHTING_H
#define BFM_VILLAGE_LIGHTING_H

#include "bedroom_figure.h"

/* The daylight phase is the same five-way selection as the lower-town sky. */
int bfm_village_lower_light(BfmBedroomFigure *figure, const uint8_t *scene,
                            size_t bytes, unsigned phase);

#endif
