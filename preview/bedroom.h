/* SPDX-License-Identifier: MIT */
#ifndef BFM_BEDROOM_H
#define BFM_BEDROOM_H
#include "bedroom_record.h"

typedef struct BfmBedroom BfmBedroom;
BfmBedroom *bfm_bedroom_open(const MusashiDiscMedia *disc, const BfmSaveRecord *record);
void bfm_bedroom_free(BfmBedroom *room);
int bfm_bedroom_step(BfmBedroom *room);
int bfm_bedroom_draw(BfmBedroom *room, uint32_t pixels[240][320]);
int bfm_bedroom_endpoint(const BfmBedroom *room);
#endif
