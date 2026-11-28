/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_ILLNESS_GROCERY_H
#define BFM_VILLAGE_ILLNESS_GROCERY_H

#include "bedroom_field.h"
#include "forest_message.h"
#include "gameplay_run.h"
#include "village_arrival_motion.h"

typedef struct BfmVillageGrocery BfmVillageGrocery;
typedef enum {
    BFM_GROCERY_ERROR = -1, BFM_GROCERY_FREE, BFM_GROCERY_LOCKED, BFM_GROCERY_RELEASED
} BfmVillageGroceryResult;

typedef struct {
    /* Transfer each opened window to the ordinary field message owner. */
    BfmForestMessage *opened;
    unsigned speaker; /* Musashi 0, Mayor Govern 378, Inn's Hotelo 379. */
    int speaker_x, portrait_on_right;
    unsigned resident_portrait; /* Room face assigned to the resident window. */
    const uint8_t *camera_station, *camera_frame;
    int release_camera;
    /* One-update commands for the two placed residents. */
    uint32_t mayor_program;
    int mayor_turn, mayor_yaw;
    unsigned hotelo_command;
} BfmVillageGroceryFrame;

/* Borrow the decoded US SC03/124 room for the life of the encounter. */
BfmVillageGrocery *bfm_village_grocery_open(const uint8_t *scene, size_t bytes);
void bfm_village_grocery_free(BfmVillageGrocery *grocery);
int bfm_village_grocery_active(const BfmVillageGrocery *grocery);
/* Signal once the street camera has completed its requested return. */
void bfm_village_grocery_camera_restored(BfmVillageGrocery *grocery);

/* Call once per ordinary field update. cast_ready means the Mayor and, before
 * story 210, Hotelo have reached their posts. Only FREE runs ordinary movement.
 * Report page_closed once the focused window finishes its final close. */
BfmVillageGroceryResult bfm_village_grocery_step(BfmVillageGrocery *grocery,
    BfmVillageMotion *motion, const BfmBedroomView *view, int square_edge, int cast_ready,
    int page_closed, BfmSaveRecord *save, BfmGameplayRun *run, BfmVillageGroceryFrame *frame);

#endif
