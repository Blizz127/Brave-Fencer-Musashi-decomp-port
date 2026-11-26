/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_GLOW_HALO_H
#define BFM_FOREST_GLOW_HALO_H

#include "forest_glows.h"

_Static_assert(BFM_FOREST_HALO_CARDS == 4 * BFM_FOREST_GLOWS, "four quads per emitting light");

/* Order the native cards with the room geometry, using this update's camera. */
unsigned bfm_forest_glows_project(const BfmForestGlows *lights, const BfmForestCamera *camera,
                                  BfmForestDustCard out[BFM_FOREST_HALO_CARDS]);
#endif
