/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_INTERIOR_H
#define BFM_TOWER_INTERIOR_H

#include "forest_field.h"
#include "room_descriptor.h"

typedef struct BfmTowerInterior BfmTowerInterior;

BfmTowerInterior *bfm_tower_interior_open(const MusashiDiscMedia *disc,
                                          const RoomTransferDescriptor *transfer);
void bfm_tower_interior_free(BfmTowerInterior *interior);
BfmForestField *bfm_tower_interior_room(BfmTowerInterior *interior);
int bfm_tower_interior_place(BfmTowerInterior *interior, ForestPlayerState *player,
                             int16_t *yaw, int16_t position[3]);
unsigned bfm_tower_interior_intro_updates(const BfmTowerInterior *interior);
/* The room-entry controller counts one authored camera key per scene update.
 * The final key holds for the message. The presenter releases the follow shot
 * when the last page has closed. */
int bfm_tower_interior_camera(const BfmTowerInterior *interior, const int16_t player[3],
                              unsigned shot_update, int follow_player,
                              BfmForestCamera *camera, int32_t eye[3]);
/* First update registers the four hanging lights with the running game. */
void bfm_tower_interior_tick(BfmTowerInterior *interior, BfmGameplayRun *run);
int bfm_tower_interior_draw(const BfmTowerInterior *interior, const BfmForestCamera *camera,
                            const BfmForestPose *pose, const BfmForestTransform *root,
                            uint32_t visible_parts, unsigned buffer, const BfmForestBladeFrame *blade,
                            uint32_t pixels[240][320]);
#endif
