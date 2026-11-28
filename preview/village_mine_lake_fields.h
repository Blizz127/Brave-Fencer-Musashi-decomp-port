/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_LAKE_FIELDS_H
#define BFM_VILLAGE_MINE_LAKE_FIELDS_H

#include "bedroom_figure.h"
#include "gameplay_run.h"
#include "village_ground.h"
#include "woods_caster_melee.h"

typedef struct BfmVillageMineLakeFields BfmVillageMineLakeFields;

/* The mounted SC03/110 scene must outlive its actors. */
BfmVillageMineLakeFields *bfm_village_mine_lake_fields_open(const uint8_t *scene, size_t bytes,
                                                            const uint8_t variables[256]);
void bfm_village_mine_lake_fields_free(BfmVillageMineLakeFields *fields);
void bfm_village_mine_lake_fields_step(BfmVillageMineLakeFields *fields, BfmGameplayRun *run,
                                      const int16_t player[3], unsigned field_tick);
int bfm_village_mine_lake_fields_body(const BfmVillageMineLakeFields *fields,
    const int16_t previous[3], int16_t proposed[3], unsigned field_tick, int airborne);

/* Query the sealed fields before their actor update. Return the resident ID and
 * intersection of a wall-clipped, accepted blade segment. The room owns the
 * subsequent mode-1 release script and the eventual rescue registration. */
unsigned bfm_village_mine_lake_fields_hit(const BfmVillageMineLakeFields *fields,
    const BfmVillageGround *ground, const BfmWoodsCasterMeleeFrame *swing, int16_t hit[3]);
int bfm_village_mine_lake_fields_draw(void *fields, BfmBedroomFigure *figure,
                                     const BfmBedroomView *view);
#endif
