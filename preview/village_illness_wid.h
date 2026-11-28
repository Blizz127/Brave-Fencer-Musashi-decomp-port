/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_ILLNESS_WID_H
#define BFM_VILLAGE_ILLNESS_WID_H

#include "village_illness_grocery.h"
#include "village_illness_windmill.h"

typedef struct BfmVillageWid BfmVillageWid;

typedef struct {
    BfmForestMessage *opened;
    unsigned speaker, resident_portrait; /* Musashi 0 or Wid 380. */
    int speaker_x, portrait_on_right;
    int start_camera, camera_eye[3], camera_target[3], release_camera;
    BfmVillageWindmillCue cast;
    /* A full inventory suspends the field until its actual return. */
    int request_inventory, pickup_effect, key_received;
    unsigned sound;
} BfmVillageWidFrame;

BfmVillageWid *bfm_village_wid_open(const uint8_t *scene, size_t bytes);
void bfm_village_wid_free(BfmVillageWid *wid);
int bfm_village_wid_active(const BfmVillageWid *wid);
/* Only the resident encounter takes Wid off his ordinary home schedule. */
int bfm_village_wid_windmill_active(const BfmVillageWid *wid);
void bfm_village_wid_camera_restored(BfmVillageWid *wid);

/* Service after the camera and before updating the resident. The cast supplies
 * the actual closed-door eligibility and resident position. Acknowledge each
 * focused window and the forced item menu only on their completed closes. */
BfmVillageGroceryResult bfm_village_wid_step(BfmVillageWid *wid,
    const BfmVillageIllnessWindmill *cast, BfmVillageMotion *motion, const BfmBedroomView *view,
    int square_edge, int page_closed, int inventory_closed, BfmSaveRecord *save,
    BfmGameplayRun *run, BfmVillageWidFrame *frame);

#endif
