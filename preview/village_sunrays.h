/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_SUNRAYS_H
#define BFM_VILLAGE_SUNRAYS_H

#include "gameplay_run.h"
#include <stddef.h>
#include <stdint.h>

typedef struct BfmVillageSunrays BfmVillageSunrays;

BfmVillageSunrays *bfm_village_sunrays_open(const MusashiDiscMedia *disc,
                                            const uint8_t *scene, size_t bytes);
void bfm_village_sunrays_free(BfmVillageSunrays *rays);

/* Advance once per lower-village static actor update, including room entry. */
void bfm_village_sunrays_step(BfmVillageSunrays *rays, BfmGameplayRun *run, unsigned hour);

/* Composite the near screen-space light after the field and before the HUD. */
void bfm_village_sunrays_draw(const BfmVillageSunrays *rays, uint32_t pixels[240][320]);

#endif
