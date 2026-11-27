/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_RESIDENT_GLOW_ART_H
#define BFM_WOODS_RESIDENT_GLOW_ART_H

#include "bedroom_figure.h"

typedef struct BfmWoodsResidentGlowArt BfmWoodsResidentGlowArt;

BfmWoodsResidentGlowArt *bfm_woods_resident_glow_art_load(const uint8_t *scene, size_t size);
void bfm_woods_resident_glow_art_free(BfmWoodsResidentGlowArt *art);

/* The actor supplies the crystal's world origin (including its placement
 * adjustment), model yaw and light angles. Flash widens the ground glow
 * and changes the crystal's blend for the current actor update. The birth
 * field has room ambient light only, until its first directional update. */
int bfm_woods_resident_glow_art_add(const BfmWoodsResidentGlowArt *art, BfmBedroomFigure *figure,
    const BfmBedroomView *view, const int16_t root[3], unsigned yaw, unsigned light_pitch,
    unsigned light_yaw, int flash, int lights_active);

#endif
