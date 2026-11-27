/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_LENO_DELIVERY_H
#define BFM_VILLAGE_LENO_DELIVERY_H

#include "bedroom_figure.h"
#include "bedroom_record.h"
#include "forest_message.h"
#include "village_arrival_motion.h"
#include "village_camera.h"

typedef struct BfmVillageLenoDelivery BfmVillageLenoDelivery;

int bfm_village_leno_delivery_pending(const BfmSaveRecord *save, int carrying);
/* Open on the upper field's first incoming actor update. Carrying is the live
 * woods actor's attachment at the marked exit. The record and scene outlive
 * the event. A successful admission records the dog's arrival immediately. */
BfmVillageLenoDelivery *bfm_village_leno_delivery_open(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes, BfmSaveRecord *save, int carrying,
    const BfmBedroomRoot *arrival);
void bfm_village_leno_delivery_free(BfmVillageLenoDelivery *delivery);

/* Run once per incoming field update, including the reveal. A true ready
 * signal hands the player to the story script. The result is zero on error,
 * one for ordinary movement or two when the script owns this update. */
int bfm_village_leno_delivery_step(BfmVillageLenoDelivery *delivery, BfmVillageMotion *motion,
    const BfmVillageCamera *camera, int player_ready, BfmForestMessage **opened);
int bfm_village_leno_delivery_active(const BfmVillageLenoDelivery *delivery);
int bfm_village_leno_delivery_message_closed(BfmVillageLenoDelivery *delivery);
/* Apply the mounted speaking gesture to a copy of the ordinary field pose. */
void bfm_village_leno_delivery_pose(const BfmVillageLenoDelivery *delivery, BfmBedroomPose *frame);
int bfm_village_leno_delivery_add(const BfmVillageLenoDelivery *delivery, BfmBedroomFigure *figure,
                                  const BfmBedroomView *view);
#endif
