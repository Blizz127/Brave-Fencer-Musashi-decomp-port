/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_MINI_SMOKE_H
#define BFM_WOODS_MINI_SMOKE_H

#include "bedroom_figure.h"
#include "hero_smoke_material.h"

typedef struct {
    BfmHeroSmokeMaterial material;
    int16_t x, z;
    int64_t y_q16;
    int32_t rise_q16;
    unsigned age;
} BfmWoodsMiniSmoke;

int bfm_woods_mini_smoke_load(BfmWoodsMiniSmoke *smoke, const uint8_t *main_data, size_t bytes);

/* Call once per effect update. Only the first application of Mini pulses
 * applied_mini; loading or refreshing an existing Mini status leaves it clear. */
void bfm_woods_mini_smoke_step(BfmWoodsMiniSmoke *smoke, int applied_mini,
                              const int16_t feet[3], const int eye[3]);
int bfm_woods_mini_smoke_add(const BfmWoodsMiniSmoke *smoke, BfmBedroomFigure *figure,
                            const BfmBedroomView *view);

#endif
