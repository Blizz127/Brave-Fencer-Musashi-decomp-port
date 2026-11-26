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
void bfm_tower_field_tick(BfmTowerField *tower, const int16_t player[3], BfmGameplayRun *gameplay);
int bfm_tower_field_camera(const BfmTowerField *tower, const int16_t player[3],
                           BfmForestCamera *out, int32_t eye[3]);
/* Draw the mounted sky, field geometry, figures and world-space effects.
 * field_tick counts room-controller updates, starting when the figures register. */
int bfm_tower_field_draw(BfmTowerField *tower, const BfmForestCamera *camera,
                         const BfmForestPose *pose, const BfmForestTransform *root,
                         uint32_t visible_parts, unsigned field_tick, const BfmForestBladeFrame *blade,
                         uint32_t pixels[240][320]);
#endif
