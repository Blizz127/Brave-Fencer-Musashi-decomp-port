/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_LOWER_H
#define BFM_VILLAGE_LOWER_H
#include "village_arrival.h"
typedef struct BfmVillageLower BfmVillageLower;
BfmVillageLower *bfm_village_lower_open(const MusashiDiscMedia *disc, BfmBedroomEntry *entry,
                                       const BfmVillageMotion *departed, uint16_t tiles[512][1024]);
void bfm_village_lower_free(BfmVillageLower *lower);
const BfmVillagePlayer *bfm_village_lower_player(const BfmVillageLower *lower);
int bfm_village_lower_step(BfmVillageLower *lower, int x, int y, int square,
                          BfmForestMessage **opened);
void bfm_village_lower_close(BfmVillageLower *lower, unsigned answer);
int bfm_village_lower_draw(BfmVillageLower *lower, const uint8_t flags[64],
                          const BfmForestMessageFrame *window, uint32_t pixels[240][320]);
#endif
