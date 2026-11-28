/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_ENTRANCE_H
#define BFM_VILLAGE_MINE_ENTRANCE_H

#include "bedroom_record.h"
#include "field_input.h"
#include "forest_message.h"
#include "gameplay_run.h"
#include "village_arrival_motion.h"

typedef struct BfmVillageMineEntrance BfmVillageMineEntrance;

/* The mine rooms borrow the surface, resident player data and live entry. */
BfmVillageMineEntrance *bfm_village_mine_entrance_open(const MusashiDiscMedia *disc,
    BfmBedroomEntry *entry, const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024]);
/* The loader supplies its current actor clock when placing the next field. */
BfmVillageMineEntrance *bfm_village_mine_entrance_open_at(const MusashiDiscMedia *disc,
    BfmBedroomEntry *entry, const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024],
    unsigned world_tick);
void bfm_village_mine_entrance_free(BfmVillageMineEntrance *mine);
uint16_t bfm_village_mine_entrance_room(const BfmVillageMineEntrance *mine);
const BfmVillagePlayer *bfm_village_mine_entrance_player(const BfmVillageMineEntrance *mine);
const BfmVillageMotion *bfm_village_mine_entrance_departure(const BfmVillageMineEntrance *mine);
int bfm_village_mine_entrance_step(BfmVillageMineEntrance *mine, BfmGameplayRun *run,
    BfmFieldInput input, unsigned actor_tick);
/* The first departure update starts the portal actor's virtual walk tape.
 * Use neutral ordinary updates while the host prepares this station. */
int bfm_village_mine_entrance_step_departure(BfmVillageMineEntrance *mine, BfmGameplayRun *run,
    unsigned actor_tick);
int bfm_village_mine_entrance_draw(BfmVillageMineEntrance *mine, uint32_t pixels[240][320]);

const BfmForestMessageFrame *bfm_village_mine_entrance_page(const BfmVillageMineEntrance *mine);
int bfm_village_mine_entrance_draw_window(const BfmVillageMineEntrance *mine, uint32_t pixels[240][320]);

#endif
