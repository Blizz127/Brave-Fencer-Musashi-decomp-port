/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_SUMMIT_H
#define BFM_TOWER_SUMMIT_H

#include "forest_field.h"
#include "tower_summit_camera.h"
#include "tower_escape.h"
#include "tower_escape_camera.h"
#include "tower_summit_arrival.h"
#include "tower_summit_lesson.h"
#include "tower_summit_residents.h"
#include "tower_summit_stun.h"
#include "tower_summit_pickup.h"

typedef struct BfmTowerSummit BfmTowerSummit;

/* Mount entrance zero from the earned lift transfer. Other summit entrances
 * have their own camera and player scripts. */
BfmTowerSummit *bfm_tower_summit_open(const MusashiDiscMedia *disc,
                                      const RoomTransferDescriptor *destination);
void bfm_tower_summit_free(BfmTowerSummit *summit);
BfmForestField *bfm_tower_summit_room(BfmTowerSummit *summit);
int bfm_tower_summit_targets(const BfmTowerSummit *summit, int16_t plate[3], int16_t sword[3]);
void bfm_tower_summit_present_sword(BfmTowerSummit *summit, unsigned tick, unsigned settle,
                                    int shielded, int sword_present);
/* Supply actual plate occupancy and pickup actor lifetime once per room update. */
void bfm_tower_summit_update_sword(BfmTowerSummit *summit, unsigned tick,
                                   int plate_occupied, int sword_present);
int bfm_tower_summit_place(BfmTowerSummit *summit, ForestPlayerState *player,
                           int16_t *yaw, int16_t position[3]);
/* Begin once after placing the actor. Advance voice transport before each
 * update. Player input resumes after the final page's dismissal update. */
int bfm_tower_summit_begin(BfmTowerSummit *summit, uint32_t parts, BfmForestPose *pose);
int bfm_tower_summit_tick_arrival(BfmTowerSummit *summit, ForestVoice *voice, unsigned *generation,
                                  int entrance_ready, int confirm, int reveal, BfmForestPose *pose);
const BfmTowerSummitArrival *bfm_tower_summit_arriving(const BfmTowerSummit *summit);
/* The pressure fixture and its conversations belong to this room visit. */
BfmTowerSummitPickup *bfm_tower_summit_pickup(BfmTowerSummit *summit);
BfmTowerSummitResidents *bfm_tower_summit_residents(BfmTowerSummit *summit);
int bfm_tower_summit_step_fixtures(BfmTowerSummit *summit, const int16_t player[3],
                                   const int32_t eye[3], BfmGameplayRun *run, int scripted, uint32_t parts);
/* Run before the resident update. The returned visual frame is owned by the
 * current room and is valid until its next skill update. */
const BfmSummitSkillFrame *bfm_tower_summit_skill_tick(BfmTowerSummit *summit,
    uint32_t animation_table, const BfmForestTransform *root, int16_t yaw, unsigned tick,
    const BfmSummitSkillInput *input, BfmGameplayRun *run);
BfmTowerSummitLesson *bfm_tower_summit_active_lesson(BfmTowerSummit *summit);
/* Continue within the current visit. Escape owns the restored player placement. */
int bfm_tower_summit_resume_escape(BfmTowerSummit *summit, BfmTowerEscape *escape,
                                   BfmTowerEscapeCamera *camera);
int bfm_tower_summit_road_tick(BfmTowerSummit *summit, const BfmTowerEscape *escape);
int bfm_tower_summit_draw(const BfmTowerSummit *summit, const BfmForestCamera *camera,
                          const BfmForestPose *pose, const BfmForestTransform *root,
                          uint32_t visible_parts, unsigned buffer, const BfmForestBladeFrame *blade,
                          uint32_t pixels[240][320]);
#endif
