/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_IMPACT_H
#define BFM_VILLAGE_IMPACT_H
#include "bedroom_field.h"
#include "forest_dust.h"
#include "gameplay_run.h"

enum { BFM_VILLAGE_IMPACT_CARDS = 54 };
typedef struct BfmVillageImpact BfmVillageImpact;

/* Expanded SC03/2 and the resident MAIN/10 type-0x101 payload. The buffers
 * may be released once the room's effect records have been decoded. */
BfmVillageImpact *bfm_village_impact_open(const uint8_t *scene, size_t scene_bytes,
                                         const uint8_t *main, size_t main_bytes);
void bfm_village_impact_free(BfmVillageImpact *impact);
/* Step once per room update with the motion player's HIT event. Returns 1
 * once when the impact starts, for the associated room sound request. */
int bfm_village_impact_step(BfmVillageImpact *impact, int hit, BfmGameplayRun *run);
/* Add to both the eye and target Y when composing this update's camera. */
int bfm_village_impact_shake_y(const BfmVillageImpact *impact);
/* Ascending slot order. Insert into the field ordering buckets and draw
 * with the resident MAIN/10 tiles followed by the SC03/2 tile upload. */
unsigned bfm_village_impact_project(const BfmVillageImpact *impact, const BfmBedroomView *view,
                                    BfmForestDustCard out[BFM_VILLAGE_IMPACT_CARDS]);
#endif
