/* SPDX-License-Identifier: MIT */
#ifndef BFM_BEDROOM_H
#define BFM_BEDROOM_H
#include "bedroom_record.h"

typedef struct BfmBedroom BfmBedroom;
BfmBedroom *bfm_bedroom_open(const MusashiDiscMedia *disc, const BfmSaveRecord *record);
void bfm_bedroom_free(BfmBedroom *room);
enum { BFM_BEDROOM_ADVANCED = 1, BFM_BEDROOM_VOICE_STOPPED };
int bfm_bedroom_step(BfmBedroom *room);
/* A stop result asks the desktop to cancel the queued voice output. */
int bfm_bedroom_step_input(BfmBedroom *room, int confirm_edge, int reveal_edge);
int bfm_bedroom_draw(BfmBedroom *room, uint32_t pixels[240][320]);
int bfm_bedroom_endpoint(const BfmBedroom *room);
struct ForestVoice *bfm_bedroom_voice(BfmBedroom *room, unsigned *generation);
#endif
