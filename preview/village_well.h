/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_WELL_H
#define BFM_VILLAGE_WELL_H

#include "bedroom_record.h"
#include "village_arrival_motion.h"
#include "village_camera.h"
#include "forest_message.h"

typedef struct BfmVillageWell BfmVillageWell;

/* A missed Square leaves the field in control. An accepted Square takes
 * ownership of the hero until the well's shot has returned. */
int bfm_village_well_begin(BfmVillageWell **well, const uint8_t *scene, size_t bytes,
                           const BfmSaveRecord *save, const BfmBedroomRoot *player,
                           const BfmVillageCamera *view);
int bfm_village_well_step(BfmVillageWell **well, BfmVillageMotion *motion,
                          BfmVillageCamera *view, BfmSaveRecord *save,
                          const MusashiDiscMedia *disc, BfmForestMessage **opened,
                          int *received);
void bfm_village_well_close(BfmVillageWell *well);
void bfm_village_well_free(BfmVillageWell *well);

#endif
