/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_SUMMIT_H
#define BFM_TOWER_SUMMIT_H

#include "forest_field.h"
#include "tower_summit_camera.h"
#include "tower_summit_arrival.h"

typedef struct BfmTowerSummit BfmTowerSummit;

/* Mount entrance zero from the earned lift transfer. Other summit entrances
 * have their own camera and player scripts. */
BfmTowerSummit *bfm_tower_summit_open(const MusashiDiscMedia *disc,
                                      const RoomTransferDescriptor *destination);
void bfm_tower_summit_free(BfmTowerSummit *summit);
BfmForestField *bfm_tower_summit_room(BfmTowerSummit *summit);
int bfm_tower_summit_place(BfmTowerSummit *summit, ForestPlayerState *player,
                           int16_t *yaw, int16_t position[3]);
/* Begin once after placing the actor. Advance voice transport before each
 * update. Player input resumes after the final page's dismissal update. */
int bfm_tower_summit_begin(BfmTowerSummit *summit, uint32_t parts, BfmForestPose *pose);
int bfm_tower_summit_tick_arrival(BfmTowerSummit *summit, ForestVoice *voice, unsigned *generation,
                                  int entrance_ready, int confirm, int reveal, BfmForestPose *pose);
const BfmTowerSummitArrival *bfm_tower_summit_arriving(const BfmTowerSummit *summit);
int bfm_tower_summit_draw(const BfmTowerSummit *summit, const BfmForestCamera *camera,
                          const BfmForestPose *pose, const BfmForestTransform *root,
                          uint32_t visible_parts, unsigned buffer, const BfmForestBladeFrame *blade,
                          uint32_t pixels[240][320]);
#endif
