/* SPDX-License-Identifier: MIT */
#ifndef BFM_BEDROOM_DUST_H
#define BFM_BEDROOM_DUST_H

#include "bedroom_field.h"
#include "hero_smoke_material.h"

typedef struct {
    int64_t position[3], velocity[3];
    unsigned age;
} BfmBedroomPuff;
typedef struct {
    BfmHeroSmokeMaterial sprite;
    BfmBedroomPuff puffs[3];
} BfmBedroomDust;
void bfm_bedroom_dust_step(BfmBedroomDust *dust, const int16_t origin[3], unsigned yaw,
                           const int eye[3], int emit);
int bfm_bedroom_puff_depth(const BfmBedroomPuff *puff, const BfmBedroomView *view);
void bfm_bedroom_puff_draw(const BfmBedroomDust *dust, unsigned index, const BfmBedroomView *view,
                           const uint16_t tiles[512][1024], uint32_t pixels[240][320]);
#endif
