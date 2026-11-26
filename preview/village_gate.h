/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_GATE_H
#define BFM_VILLAGE_GATE_H
#include "bedroom_field.h"
typedef struct BfmVillageGate BfmVillageGate;

/* Prepare the two placed leaves at their initial story-selected headings. */
BfmVillageGate *bfm_village_gate_load(const uint8_t *scene, size_t bytes, uint32_t room,
                                      unsigned progress, const uint8_t flags[64]);
void bfm_village_gate_free(BfmVillageGate *gate);
int bfm_village_gate_prepare(BfmVillageGate *gate, const BfmBedroomView *view,
                             const uint16_t tiles[512][1024], uint32_t *pixels);
void bfm_village_gate_draw_to(BfmVillageGate *gate, int depth);
#endif
