/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_ILLNESS_LOWER_H
#define BFM_VILLAGE_ILLNESS_LOWER_H

#include "bedroom_record.h"
#include "field_input.h"
#include "forest_hud.h"
#include "forest_message.h"
#include "village_arrival_motion.h"

typedef struct BfmVillageIllnessLower BfmVillageIllnessLower;

/* Continue and the return from the inn both use the mounted entrance 10.
 * The caller keeps entry, main and tiles alive and owns the returned HUD. */
BfmVillageIllnessLower *bfm_village_illness_lower_open(const MusashiDiscMedia *disc,
    BfmBedroomEntry *entry, const uint8_t *main, size_t bytes, uint16_t tiles[512][1024],
    BfmForestHud **hud);
void bfm_village_illness_lower_free(BfmVillageIllnessLower *street);
const BfmVillagePlayer *bfm_village_illness_lower_player(const BfmVillageIllnessLower *street);
/* The descriptor borrows the mounted street for the duration of the transfer. */
const uint8_t *bfm_village_illness_lower_door(const BfmVillageIllnessLower *street);
int bfm_village_illness_lower_step(BfmVillageIllnessLower *street, BfmGameplayRun *run,
                                  BfmFieldInput input, unsigned scene_tick);
int bfm_village_illness_lower_rest_hud(const BfmVillageIllnessLower *street, BfmForestHud *hud);
int bfm_village_illness_lower_draw(BfmVillageIllnessLower *street, unsigned scene_tick,
    const uint8_t flags[64], unsigned draw_page, uint32_t pixels[240][320]);

const BfmForestMessageFrame *bfm_village_illness_lower_page(const BfmVillageIllnessLower *street);
int bfm_village_illness_lower_draw_window(const BfmVillageIllnessLower *street, uint32_t pixels[240][320]);
int bfm_village_illness_lower_bag_active(const BfmVillageIllnessLower *street);
int bfm_village_illness_lower_menu_serviced(const BfmVillageIllnessLower *street);
unsigned bfm_village_illness_lower_take_score_command(BfmVillageIllnessLower *street);
unsigned bfm_village_illness_lower_take_sounds(BfmVillageIllnessLower *street, unsigned sounds[2]);

#endif
