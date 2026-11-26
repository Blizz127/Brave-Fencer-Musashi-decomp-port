/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_ARRIVAL_MOTION_H
#define BFM_VILLAGE_ARRIVAL_MOTION_H

#include "bedroom_motion.h"
#include "bedroom_travel.h"

typedef struct BfmVillageMotion BfmVillageMotion;

typedef enum {
    BFM_VILLAGE_RIDE, BFM_VILLAGE_DROP, BFM_VILLAGE_IMPACT,
    BFM_VILLAGE_SETTLE, BFM_VILLAGE_WAIT_MUSIC, BFM_VILLAGE_TURN,
    BFM_VILLAGE_FIRST_WALK, BFM_VILLAGE_FIRST_MESSAGE,
    BFM_VILLAGE_SECOND_WALK, BFM_VILLAGE_SECOND_MESSAGE, BFM_VILLAGE_CONTROL
} BfmVillagePhase;

enum {
    BFM_VILLAGE_HIT = 1u, BFM_VILLAGE_CAMERA = 2u, BFM_VILLAGE_OPEN_MESSAGE = 4u,
    BFM_VILLAGE_SHOW_HUD = 8u, BFM_VILLAGE_RELEASE = 16u
};

typedef struct {
    BfmBedroomRoot root;
    BfmBedroomPose pose;
    BfmVillagePhase phase;
    unsigned controlled, events;
    uint32_t message;
    unsigned movement; /* Cable updates since mounting, retained through impact. */
} BfmVillagePlayer;

/* Borrow the unpacked SC03/2 room and the resident MAIN/10 type-0x101 payload. */
BfmVillageMotion *bfm_village_motion_open(const uint8_t *scene, size_t scene_bytes,
    const uint8_t *main, size_t main_bytes, uint32_t room, unsigned entrance);
const BfmVillagePlayer *bfm_village_motion_player(const BfmVillageMotion *motion);
/* Once per gameplay update. Pad axes are -127..127, right and down positive.
 * Camera yaw is target minus eye in X/Z, with +Z at zero. */
int bfm_village_motion_step(BfmVillageMotion *motion, int stick_x, int stick_y, int camera_yaw);
/* Signal music readiness or closure of the current message before the next step. */
int bfm_village_motion_continue(BfmVillageMotion *motion);
/* Start only after the resident has accepted Talk. The mounted conversation
 * specifies the walking offset in the resident's local coordinates. */
int bfm_village_motion_begin_talk(BfmVillageMotion *motion, const int16_t resident_xyz[3],
                                  unsigned resident_yaw);
/* Pass the previous view's eye and target while its conversation zoom moves. */
int bfm_village_motion_view_yaw(const int32_t eye[3], const int32_t target[3]);
int bfm_village_motion_talk_ready(const BfmVillageMotion *motion);
int bfm_village_motion_end_talk(BfmVillageMotion *motion);
int bfm_village_motion_face(const BfmVillageMotion *motion, uint16_t tiles[512][1024]);
void bfm_village_motion_free(BfmVillageMotion *motion);

#endif
