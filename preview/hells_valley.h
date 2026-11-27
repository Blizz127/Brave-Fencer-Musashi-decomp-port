/* SPDX-License-Identifier: MIT */
#ifndef BFM_HELLS_VALLEY_H
#define BFM_HELLS_VALLEY_H

#include "bedroom_record.h"
#include "field_input.h"
#include "forest_hud.h"
#include "gameplay_run.h"
#include "retry_screen.h"
#include "village_arrival_motion.h"

typedef struct BfmHellsValley BfmHellsValley;

/* The receiver borrows the canonical field entry, player data and VRAM.
 * The caller stages tile changes until the first field has been drawn. */
BfmHellsValley *bfm_hells_valley_open(const MusashiDiscMedia *disc, BfmBedroomEntry *entry,
    const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024]);
void bfm_hells_valley_free(BfmHellsValley *valley);
const BfmVillagePlayer *bfm_hells_valley_player(const BfmHellsValley *valley);
const BfmVillageMotion *bfm_hells_valley_departure(const BfmHellsValley *valley);
int bfm_hells_valley_step(BfmHellsValley *valley, BfmGameplayRun *run, BfmFieldInput input);
int bfm_hells_valley_step_at(BfmHellsValley *valley, BfmGameplayRun *run, BfmFieldInput input,
                            unsigned actor_tick);
size_t bfm_hells_valley_take_sounds(BfmHellsValley *valley, unsigned *out, size_t capacity);
int bfm_hells_valley_message_closed(BfmHellsValley *valley);
int bfm_hells_valley_detector(const BfmHellsValley *valley);
int bfm_hells_valley_hud(BfmHellsValley *valley, BfmForestHud *hud);
int bfm_hells_valley_draw(BfmHellsValley *valley, const uint8_t flags[64],
                         uint32_t pixels[240][320]);

int bfm_hells_valley_step_with_notice(BfmHellsValley *valley, BfmGameplayRun *run, BfmFieldInput input,
                                    unsigned actor_tick, BfmForestMessage **opened);
BfmRetryScreen *bfm_hells_valley_retry(const BfmHellsValley *valley);
int bfm_hells_valley_draw_retry(BfmHellsValley *valley, const BfmForestHud *hud, uint32_t pixels[240][320]);
int bfm_hells_valley_draw_notice(BfmHellsValley *valley, const BfmForestMessageFrame *notice,
                                uint32_t pixels[240][320]);
int bfm_hells_valley_recover(const BfmHellsValley *valley, const BfmGameplayRun *run, BfmBedroomEntry *out);

#endif
