/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_FIELD_H
#define BFM_TOWER_FIELD_H

#include "forest_field.h"
#include "room_descriptor.h"
#include "gameplay_run.h"

typedef struct BfmTowerField BfmTowerField;

BfmTowerField *bfm_tower_field_open(const MusashiDiscMedia *disc,
                                    const RoomTransferDescriptor *entry);
void bfm_tower_field_free(BfmTowerField *tower);
/* Normal player movement uses this room's mounted floor and wall grid. */
BfmForestField *bfm_tower_field_room(BfmTowerField *tower);
int bfm_tower_field_place(BfmTowerField *tower, ForestPlayerState *player,
                          int16_t *model_yaw, int16_t cached[3]);
void bfm_tower_field_tick(BfmTowerField *tower, const int16_t player[3], int facing, BfmGameplayRun *gameplay);
/* Each physical swing supplies a fresh nonzero stroke ID for all its rays.
 * A success clips the segment to the mounted column and returns a Q12 normal. */
int bfm_tower_field_blade_contact(BfmTowerField *tower, unsigned stroke_id,
                                  const int16_t start[3], const int16_t end[3],
                                  int16_t contact[3], int16_t normal[3]);
int bfm_tower_field_body_slide(const BfmTowerField *tower, const int16_t from[3],
                               const int16_t to[3], int16_t contact[3]);
/* Acknowledge when the player dismisses the presenter's final page. */
int bfm_tower_field_plate_note(const BfmTowerField *tower);
void bfm_tower_field_close_plate_note(BfmTowerField *tower);
/* The reveal advances through drain, lift, summit, then returned control. */
unsigned bfm_tower_field_rise_phase(const BfmTowerField *tower);
/* Raised becomes true after the full lift and its closing camera hold. */
int bfm_tower_field_raised(const BfmTowerField *tower);
int bfm_tower_field_revealing(const BfmTowerField *tower);
int bfm_tower_field_plate_pressing(const BfmTowerField *tower);
unsigned bfm_tower_field_plate_count(const BfmTowerField *tower);
int bfm_tower_field_camera(const BfmTowerField *tower, const int16_t player[3],
                           BfmForestCamera *out, int32_t eye[3]);
/* Draw the mounted sky, field geometry, figures and world-space effects.
 * field_tick counts room-controller updates, starting when the figures register. */
int bfm_tower_field_draw(BfmTowerField *tower, const BfmForestCamera *camera,
                         const BfmForestPose *pose, const BfmForestTransform *root,
                         uint32_t visible_parts, unsigned field_tick, const BfmForestBladeFrame *blade,
                         uint32_t pixels[240][320]);
#endif
