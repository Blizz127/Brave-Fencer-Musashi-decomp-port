/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_LOWER_H
#define BFM_VILLAGE_LOWER_H
#include "forest_hud.h"
#include "village_arrival.h"
typedef struct BfmVillageLower BfmVillageLower;
BfmVillageLower *bfm_village_lower_open(const MusashiDiscMedia *disc, BfmBedroomEntry *entry,
                                       const BfmVillageMotion *departed, uint16_t tiles[512][1024]);
BfmVillageLower *bfm_village_lower_resume(const MusashiDiscMedia *disc, BfmBedroomEntry *entry,
                                         const uint8_t *main, size_t bytes,
                                         uint16_t tiles[512][1024], BfmForestHud **hud);
/* Valid on the field update in which the controlled player presses into
 * the inn wall. The mounted descriptor lives until the lower field closes. */
const uint8_t *bfm_village_lower_inn_transfer(const BfmVillageLower *lower);
const uint8_t *bfm_village_lower_castle_transfer(const BfmVillageLower *lower);
const uint8_t *bfm_village_lower_shop_transfer(const BfmVillageLower *lower);
const BfmVillageMotion *bfm_village_lower_departure(const BfmVillageLower *lower);
void bfm_village_lower_free(BfmVillageLower *lower);
void bfm_village_lower_begin_hotelo_reveal(BfmVillageLower *lower);
int bfm_village_lower_rest_hud(const BfmVillageLower *lower, BfmForestHud *hud);
const BfmVillagePlayer *bfm_village_lower_player(const BfmVillageLower *lower);
int bfm_village_lower_step(BfmVillageLower *lower, int x, int y, int square,
                          BfmForestMessage **opened);
void bfm_village_lower_register_actors(BfmVillageLower *lower, BfmGameplayRun *run);
void bfm_village_lower_step_ambient(BfmVillageLower *lower, BfmGameplayRun *run);
int bfm_village_lower_step_gameplay(BfmVillageLower *lower, BfmGameplayRun *run,
                                   BfmFieldInput field, BfmForestMessage **opened);
int bfm_village_lower_step_field(BfmVillageLower *lower, BfmFieldInput field,
                                BfmForestMessage **opened);
unsigned bfm_village_lower_take_score_command(BfmVillageLower *lower);
void bfm_village_lower_close(BfmVillageLower *lower, unsigned answer);
int bfm_village_lower_draw(BfmVillageLower *lower, unsigned tick, const uint8_t flags[64],
                          const BfmForestMessageFrame *window, uint32_t pixels[240][320]);
#endif
