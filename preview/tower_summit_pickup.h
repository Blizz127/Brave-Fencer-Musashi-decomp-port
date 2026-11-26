/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_SUMMIT_PICKUP_H
#define BFM_TOWER_SUMMIT_PICKUP_H

#include "gameplay_run.h"
#include "forest_field.h"
#include "palace_sound_bank.h"

typedef struct BfmTowerSummitPickup BfmTowerSummitPickup;
typedef enum {
    BFM_SUMMIT_PICKUP_HOLD, BFM_SUMMIT_PICKUP_ALIGN, BFM_SUMMIT_PICKUP_TURN,
    BFM_SUMMIT_PICKUP_FOLLOWUP
} BfmSummitPickupGoal;

typedef struct {
    uint32_t visible_parts, face_resource;
    int8_t fusion_anchors[2][3], lumina_anchors[2][3];
    unsigned sound_command, score_selector;
    int has_pose;
} BfmSummitPickupFrame;
enum { BFM_SUMMIT_PICKUP_GLOW_CARDS = 10 };

/* Begin when the live sword's settled contact fires. The mounted room and
 * disc outlive the presenter. Keep the current pose on the contact update. */
BfmTowerSummitPickup *bfm_tower_summit_pickup_begin(const BfmForestField *room,
    const MusashiDiscMedia *disc, uint32_t visible_parts);
void bfm_tower_summit_pickup_free(BfmTowerSummitPickup *pickup);
/* ALIGN sends the common walker to the mounted position. TURN asks it to
 * reach the given yaw. The other phases keep input with the script. */
BfmSummitPickupGoal bfm_tower_summit_pickup_goal(const BfmTowerSummitPickup *pickup,
    int16_t target[3], int16_t *yaw);
/* Once per game update. aligned means the actor reached and snapped to the
 * target. facing means the actor entered the update on the desired yaw. */
int bfm_tower_summit_pickup_tick(BfmTowerSummitPickup *pickup, BfmGameplayRun *run,
    int aligned, int facing, BfmForestPose *pose);
const BfmSummitPickupFrame *bfm_tower_summit_pickup_frame(const BfmTowerSummitPickup *pickup);
int bfm_tower_summit_pickup_camera(const BfmTowerSummitPickup *pickup,
    const int16_t player[3], BfmForestCamera *camera, int32_t eye[3]);
/* Submit additive quads to the room's ordering table while part 25 is visible. */
size_t bfm_tower_summit_pickup_glow(const BfmForestField *room,
    const int8_t lumina_anchors[2][3], const BfmForestTransform *root,
    const int32_t eye[3], const BfmForestCamera *camera, unsigned scene_tick,
    BfmForestDustCard *out, size_t capacity);
/* The next summit script can take ownership of its ready bank. FOLLOWUP keeps
 * the player under script control. */
BfmPalaceBank *bfm_tower_summit_pickup_take_bank(BfmTowerSummitPickup *pickup);
#endif
