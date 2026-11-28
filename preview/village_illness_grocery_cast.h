/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_ILLNESS_GROCERY_CAST_H
#define BFM_VILLAGE_ILLNESS_GROCERY_CAST_H

#include "bedroom_figure.h"
#include "bedroom_record.h"

typedef struct BfmVillageIllnessGroceryCast BfmVillageIllnessGroceryCast;

/* Borrow the mounted SC03/124 room for the lifetime of the cast. Admission
 * and initial placement sample the saved story and clock on room entry. */
BfmVillageIllnessGroceryCast *bfm_village_illness_grocery_cast_open(
    const uint8_t *scene, size_t bytes, const BfmSaveRecord *save);
void bfm_village_illness_grocery_cast_free(BfmVillageIllnessGroceryCast *cast);

/* Run once per serviced actor field. The grocery script passes its new
 * animation, turn and Hotelo commands in the field where it issues them. */
int bfm_village_illness_grocery_cast_step(BfmVillageIllnessGroceryCast *cast,
    const BfmSaveRecord *save, unsigned scene_tick, int event_active,
    uint32_t mayor_program, int mayor_turn, int mayor_yaw, unsigned hotelo_command);
int bfm_village_illness_grocery_cast_ready(const BfmVillageIllnessGroceryCast *cast,
    unsigned story);
const int16_t *bfm_village_illness_grocery_cast_position(
    const BfmVillageIllnessGroceryCast *cast, unsigned actor_class);
int bfm_village_illness_grocery_cast_add(const BfmVillageIllnessGroceryCast *cast,
    BfmBedroomFigure *figure, const BfmBedroomView *view);

#endif
