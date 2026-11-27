/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_JON_STAGE_H
#define BFM_VILLAGE_JON_STAGE_H

#include "bedroom_travel.h"
#include "bedroom_motion.h"
#include "village_camera.h"

typedef struct BfmVillageJonStage BfmVillageJonStage;

typedef enum {
    BFM_JON_STAGE_ERROR = -1, BFM_JON_STAGE_WAIT, BFM_JON_STAGE_INSTALL,
    BFM_JON_STAGE_RESTORED
} BfmVillageJonStageEvent;

/* Keep the lower-village scene and the unpacked stocks event member mounted. */
BfmVillageJonStage *bfm_village_jon_stage_open(const uint8_t *scene, size_t bytes,
                    uint32_t room, const uint8_t *module, size_t module_bytes);
void bfm_village_jon_stage_free(BfmVillageJonStage *stage);
/* The room checks the ordinary talk proxy, carry state, story and inventory
 * before calling begin. An accepted Square suspends free field movement. */
int bfm_village_jon_stage_begin(BfmVillageJonStage *stage, const BfmBedroomRoot *player,
                               int square_pressed);
/* Supply the ordinary follow framing and step once per field update, including
 * the update accepting Square. member_ready reports the field loader's event
 * slot. INSTALL admits the first text and Leno greeting on this update; the
 * player walk and shot continue while the window is visible. RESTORED presents
 * the saved framing, then the next ordinary update follows the final root. */
BfmVillageJonStageEvent bfm_village_jon_stage_step(BfmVillageJonStage *stage,
                                  int member_ready, BfmVillageCamera *presented);
/* Call on the return-camera command after the final voiced window closes. */
void bfm_village_jon_stage_finish(BfmVillageJonStage *stage);
/* A paired field walk and turn run alongside the dialogue at the stocks. */
int bfm_village_jon_stage_walk(BfmVillageJonStage *stage, int local_x, int local_z);
int bfm_village_jon_stage_react(BfmVillageJonStage *stage, int affirmative);
/* One-shot body selection for the current staged movement update. */
BfmBedroomAction bfm_village_jon_stage_action(const BfmVillageJonStage *stage);
int bfm_village_jon_stage_active(const BfmVillageJonStage *stage);
const BfmBedroomRoot *bfm_village_jon_stage_player(const BfmVillageJonStage *stage);
#endif
