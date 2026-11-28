/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_ENTRANCE_H
#define BFM_VILLAGE_MINE_ENTRANCE_H

#include "bedroom_record.h"
#include "field_input.h"
#include "forest_message.h"
#include "gameplay_run.h"
#include "village_arrival_motion.h"
#include "village_mine_lake.h"
#include "retry_screen.h"

typedef struct BfmVillageMineEntrance BfmVillageMineEntrance;

/* The mine rooms borrow the surface, resident player data and live entry. */
BfmVillageMineEntrance *bfm_village_mine_entrance_open(const MusashiDiscMedia *disc,
    BfmBedroomEntry *entry, const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024]);
/* The loader supplies its current actor clock when placing the next field. */
BfmVillageMineEntrance *bfm_village_mine_entrance_open_at(const MusashiDiscMedia *disc,
    BfmBedroomEntry *entry, const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024],
    unsigned world_tick);
void bfm_village_mine_entrance_free(BfmVillageMineEntrance *mine);
/* Channel is the original wheel number, 0 through 2. */
typedef void (*BfmMineWheelSound)(void *context, unsigned command, unsigned value, unsigned channel);
void bfm_village_mine_entrance_set_wheel_sound(BfmVillageMineEntrance *mine,
    BfmMineWheelSound callback, void *context);
/* The fan and checkpoint use the shaft's mounted effect bank. */
typedef void (*BfmMineShaftSound)(void *context, unsigned command, unsigned value);
void bfm_village_mine_entrance_set_shaft_sound(BfmVillageMineEntrance *mine,
    BfmMineShaftSound callback, void *context);
void bfm_village_mine_entrance_set_lake_sound(BfmVillageMineEntrance *mine,
    BfmMineLakeSound callback, void *context);
/* Lower shaft exit admission follows the installed receiver. */
void bfm_village_mine_entrance_set_lake_exit(BfmVillageMineEntrance *mine, int enabled);
BfmRetryScreen *bfm_village_mine_entrance_retry(const BfmVillageMineEntrance *mine);
int bfm_village_mine_entrance_prepare_recovery(const BfmVillageMineEntrance *mine,
    const MusashiDiscMedia *disc, const BfmGameplayRun *run, BfmBedroomEntry *entry);
int bfm_village_mine_entrance_step_retry_world(BfmVillageMineEntrance *mine, BfmGameplayRun *run,
    unsigned actor_tick, BfmFieldInput input);
int bfm_village_mine_entrance_draw_retry(BfmVillageMineEntrance *mine, const BfmForestHud *hud,
    uint32_t pixels[240][320]);
int bfm_village_mine_entrance_rider(const BfmVillageMineEntrance *mine);
uint16_t bfm_village_mine_entrance_room(const BfmVillageMineEntrance *mine);
const BfmVillagePlayer *bfm_village_mine_entrance_player(const BfmVillageMineEntrance *mine);
const BfmVillageMotion *bfm_village_mine_entrance_departure(const BfmVillageMineEntrance *mine);
int bfm_village_mine_entrance_step(BfmVillageMineEntrance *mine, BfmGameplayRun *run,
    BfmFieldInput input, unsigned actor_tick);
/* The first departure update starts the portal actor's virtual walk tape.
 * Use neutral ordinary updates while the host prepares this station. */
int bfm_village_mine_entrance_step_departure(BfmVillageMineEntrance *mine, BfmGameplayRun *run,
    unsigned actor_tick);
unsigned bfm_village_mine_entrance_take_hit_sound(BfmVillageMineEntrance *mine);
int bfm_village_mine_entrance_draw(BfmVillageMineEntrance *mine, uint32_t pixels[240][320]);

const BfmForestMessageFrame *bfm_village_mine_entrance_page(const BfmVillageMineEntrance *mine);
int bfm_village_mine_entrance_draw_window(const BfmVillageMineEntrance *mine, uint32_t pixels[240][320]);

#endif
