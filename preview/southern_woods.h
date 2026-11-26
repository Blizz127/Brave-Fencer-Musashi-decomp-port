/* SPDX-License-Identifier: MIT */
#ifndef BFM_SOUTHERN_WOODS_H
#define BFM_SOUTHERN_WOODS_H

#include "bedroom_record.h"
#include "field_input.h"
#include "gameplay_run.h"
#include "forest_message.h"
#include "village_arrival_motion.h"

typedef struct BfmSouthernWoods BfmSouthernWoods;

BfmSouthernWoods *bfm_southern_woods_open(const MusashiDiscMedia *disc, BfmBedroomEntry *entry,
    const uint8_t *door, const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024]);
void bfm_southern_woods_free(BfmSouthernWoods *woods);
int bfm_southern_woods_step_field(BfmSouthernWoods *woods, BfmGameplayRun *run, BfmFieldInput field,
                                  BfmForestMessage **opened);
int bfm_southern_woods_tether_gauge(const BfmSouthernWoods *woods);
int bfm_southern_woods_step(BfmSouthernWoods *woods, int x, int y,
                            int talk_edge, BfmForestMessage **opened);
int bfm_southern_woods_close(BfmSouthernWoods *woods);
const BfmVillagePlayer *bfm_southern_woods_player(const BfmSouthernWoods *woods);
int bfm_southern_woods_draw(BfmSouthernWoods *woods, const uint8_t flags[64],
                            const BfmForestMessageFrame *window, uint32_t pixels[240][320]);

#endif
