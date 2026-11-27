/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MAYOR_STAGE_H
#define BFM_VILLAGE_MAYOR_STAGE_H

#include "bedroom_figure.h"
#include "bedroom_record.h"
#include "bedroom_travel.h"
#include "village_camera.h"

typedef struct BfmVillageMayorStage BfmVillageMayorStage;

/* Borrow the SC03 lower-village field and the supplemental Mayor event payload. */
BfmVillageMayorStage *bfm_village_mayor_stage_open(const uint8_t *scene, size_t bytes,
                        uint32_t room, const uint8_t *mayor, size_t mayor_bytes, uint32_t shadow);
void bfm_village_mayor_stage_free(BfmVillageMayorStage *stage);
int bfm_village_mayor_stage_begin(BfmVillageMayorStage *stage, BfmSaveRecord *state,
                   const BfmBedroomRoot *player, const BfmVillageCamera *camera, int square_edge);
/* Once per field logic tick after updating the lower-village camera. */
int bfm_village_mayor_stage_step(BfmVillageMayorStage *stage, BfmVillageCamera *normal);
/* True for one update after stepping into position to knock at the door. */
int bfm_village_mayor_stage_knock(const BfmVillageMayorStage *stage);
int bfm_village_mayor_stage_message_ready(const BfmVillageMayorStage *stage);
void bfm_village_mayor_stage_finish(BfmVillageMayorStage *stage);
int bfm_village_mayor_stage_active(const BfmVillageMayorStage *stage);
/* Retains the final root when control returns. Walk/stand edges select the body
 * action. Call position to project the Mayor while his actor is present. */
const BfmBedroomRoot *bfm_village_mayor_stage_player(const BfmVillageMayorStage *stage);
const int16_t *bfm_village_mayor_stage_position(const BfmVillageMayorStage *stage);
int bfm_village_mayor_stage_add(BfmVillageMayorStage *stage, BfmBedroomFigure *figure,
                                const BfmBedroomView *view);
/* Prepare the hinged leaf for the room ordering table, then merge it with
 * Musashi, the resident and the terrain at their actual bucket depths. */
int bfm_village_mayor_stage_prepare(BfmVillageMayorStage *stage, const BfmBedroomView *view,
                                    const uint16_t tiles[512][1024], uint32_t *pixels);
int bfm_village_mayor_stage_next_depth(const BfmVillageMayorStage *stage);
void bfm_village_mayor_stage_draw_to(BfmVillageMayorStage *stage, int depth);
#endif
