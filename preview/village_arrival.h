/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_ARRIVAL_H
#define BFM_VILLAGE_ARRIVAL_H
#include "forest_hud.h"
#include "bedroom_record.h"
#include "gameplay_run.h"
#include "forest_message.h"
#include "village_arrival_motion.h"

typedef struct BfmVillageArrival BfmVillageArrival;
BfmVillageArrival *bfm_village_arrival_open(const MusashiDiscMedia *disc, BfmBedroomEntry *entry,
                      const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024]);
BfmVillageArrival *bfm_village_resume_inn(const MusashiDiscMedia *disc, BfmBedroomEntry *entry,
                                         const uint8_t *main, size_t bytes,
                                         uint16_t tiles[512][1024], BfmForestHud **hud);
void bfm_village_arrival_free(BfmVillageArrival *arrival);
uint16_t bfm_village_arrival_room(const BfmVillageArrival *arrival);
const BfmVillagePlayer *bfm_village_arrival_player(const BfmVillageArrival *arrival);
int bfm_village_arrival_step(BfmVillageArrival *arrival, BfmGameplayRun *run, int x, int y);
int bfm_village_arrival_step_input(BfmVillageArrival *arrival, BfmGameplayRun *run,
                                   int x, int y, int talk_edge, BfmForestMessage **opened);
void bfm_village_arrival_set_scene_tick(BfmVillageArrival *arrival, unsigned tick);
const uint8_t *bfm_village_arrival_inn_transfer(const BfmVillageArrival *arrival);
unsigned bfm_village_arrival_reveal_tick(const BfmVillageArrival *arrival);
unsigned bfm_village_arrival_transfer_tick(const BfmVillageArrival *arrival);
int bfm_village_arrival_message_closed(BfmVillageArrival *arrival, unsigned answer);
int bfm_village_arrival_continue(BfmVillageArrival *arrival);
BfmForestMessage *bfm_village_arrival_speech(const BfmVillageArrival *arrival);
int bfm_village_arrival_draw(BfmVillageArrival *arrival, const uint8_t flags[64],
                             const BfmForestMessageFrame *window, uint32_t pixels[240][320]);
#endif
