/* SPDX-License-Identifier: MIT */
#ifndef BFM_HERO_SMOKE_H
#define BFM_HERO_SMOKE_H
#include "hero_smoke_material.h"

typedef void (*BfmHeroSmokeEmit)(void *context, unsigned field_age,
                                 const int screen[4][3], int bucket);
void bfm_hero_smoke_prepare(const BfmHeroSmokeMaterial *material, unsigned opening_field,
                            const int camera[3][3], const int translation[3],
                            BfmHeroSmokeEmit emit, void *context);
#endif
