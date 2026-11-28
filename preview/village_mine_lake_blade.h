/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_LAKE_BLADE_H
#define BFM_VILLAGE_MINE_LAKE_BLADE_H

#include "village_arrival_motion.h"
#include "woods_caster_melee.h"

typedef struct BfmMineLakeBlade BfmMineLakeBlade;

BfmMineLakeBlade *bfm_mine_lake_blade_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, const uint8_t *main, size_t main_bytes);
void bfm_mine_lake_blade_free(BfmMineLakeBlade *blade);
/* Call once in the regular player slot, after injury or scripted interaction
 * has received input. A claimed field cancels the ordinary sword action. */
int bfm_mine_lake_blade_player(BfmMineLakeBlade *blade, BfmVillageMotion *motion,
    BfmGameplayRun *run, const BfmSaveRecord *save, BfmFieldInput input, int pad_yaw, int claimed);
const BfmWoodsCasterMeleeFrame *bfm_mine_lake_blade_frame(const BfmMineLakeBlade *blade);
int bfm_mine_lake_blade_body(const BfmMineLakeBlade *blade, const uint8_t *main,
                             size_t main_bytes, BfmBedroomPose *pose);
/* Confirm a physical actor hit after clipping against scenery. */
void bfm_mine_lake_blade_connected(BfmMineLakeBlade *blade);

#endif
