/* SPDX-License-Identifier: MIT */
#ifndef BFM_CASTLE_CABLE_H
#define BFM_CASTLE_CABLE_H
#include "bedroom_record.h"
#include "gameplay_run.h"
typedef struct BfmCastleCable BfmCastleCable;
BfmCastleCable *bfm_castle_cable_open(const MusashiDiscMedia *disc, BfmBedroomEntry *entry,
                 const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024]);
void bfm_castle_cable_free(BfmCastleCable *cable);
int bfm_castle_cable_step(BfmCastleCable *cable, const uint8_t flags[64], BfmGameplayRun *run,
                         uint8_t destination[36], uint32_t pixels[240][320]);
#endif
