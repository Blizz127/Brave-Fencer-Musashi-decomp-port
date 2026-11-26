/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_RESIDENTS_H
#define BFM_VILLAGE_RESIDENTS_H

#include "bedroom_figure.h"

typedef struct BfmVillageResidents BfmVillageResidents;

/* Earned File 1, hillside entrance 5. Keep the mounted SC03 member alive
 * until the residents are freed. The figure queue supplies the room lighting
 * and depth ordering shared with Musashi and the terrain. */
BfmVillageResidents *bfm_village_residents_load(const uint8_t *scene, size_t bytes,
                                                 uint32_t room, uint32_t shadow);
void bfm_village_residents_free(BfmVillageResidents *residents);

/* Call after preparing Musashi's figure for the current view. Age counts
 * village logic updates, independently of the number of presented frames. */
int bfm_village_residents_add(const BfmVillageResidents *residents, BfmBedroomFigure *figure,
                              const BfmBedroomView *view, unsigned age);
#endif
