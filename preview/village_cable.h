/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_CABLE_H
#define BFM_VILLAGE_CABLE_H
#include "bedroom_field.h"
#include "gameplay_run.h"

typedef struct BfmVillageCable BfmVillageCable;
BfmVillageCable *bfm_village_cable_load(const uint8_t *scene, size_t bytes, uint32_t room);
void bfm_village_cable_free(BfmVillageCable *cable);
/* Advance once per game update. Emission uses the shared game clock and RNG. */
void bfm_village_cable_step(BfmVillageCable *cable, unsigned tick, int emitting,
                            const int16_t hero_xyz[3], BfmGameplayRun *run);
int bfm_village_cable_prepare(BfmVillageCable *cable, const BfmBedroomView *view,
                              const int16_t hero_xyz[3], int riding,
                              const uint16_t tiles[512][1024], uint32_t *pixels);
/* The next occupied ordering-table bucket, or INT_MIN after the last face. */
int bfm_village_cable_next_depth(const BfmVillageCable *cable);
void bfm_village_cable_draw_to(BfmVillageCable *cable, int depth);
#endif
