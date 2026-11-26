/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_FIELD_H
#define BFM_VILLAGE_FIELD_H
#include "bedroom_figure.h"
#include "bedroom_motion.h"
#include "bedroom_travel.h"
#include "village_camera.h"

typedef struct {
    BfmBedroomField *field;
    BfmBedroomFigure *hero;
    BfmForestBlade fusion;
    const int8_t *guard, *blade;
} BfmVillageField;

/* The expanded SC03 village member stays mounted while the field is active. */
int bfm_village_field_load(BfmVillageField *field, const MusashiDiscMedia *disc,
                           const uint8_t *scene, size_t size, uint32_t room);
/* Street and mayor's porch, mounted from SC03 member 1. */
int bfm_village_field_load_lower(BfmVillageField *field, const MusashiDiscMedia *disc,
                                 const uint8_t *scene, size_t size, uint32_t room);
void bfm_village_field_clear(BfmVillageField *field);
int bfm_village_field_prepare(BfmVillageField *field, const BfmVillageCamera *camera,
                              const BfmBedroomRoot *root, const BfmBedroomPose *pose,
                              const uint8_t flags[64], unsigned field_age,
                              uint16_t tiles[512][1024], uint32_t *pixels);
#endif
