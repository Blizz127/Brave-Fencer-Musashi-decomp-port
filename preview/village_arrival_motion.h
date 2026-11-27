/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_ARRIVAL_MOTION_H
#define BFM_VILLAGE_ARRIVAL_MOTION_H

#include "bedroom_motion.h"
#include "bedroom_travel.h"
#include "forest_dust.h"

typedef struct BfmVillageMotion BfmVillageMotion;

typedef enum {
    BFM_VILLAGE_RIDE, BFM_VILLAGE_DROP, BFM_VILLAGE_IMPACT,
    BFM_VILLAGE_SETTLE, BFM_VILLAGE_WAIT_MUSIC, BFM_VILLAGE_TURN,
    BFM_VILLAGE_FIRST_WALK, BFM_VILLAGE_FIRST_MESSAGE,
    BFM_VILLAGE_SECOND_WALK, BFM_VILLAGE_SECOND_MESSAGE, BFM_VILLAGE_CONTROL
} BfmVillagePhase;

enum {
    BFM_VILLAGE_HIT = 1u, BFM_VILLAGE_CAMERA = 2u, BFM_VILLAGE_OPEN_MESSAGE = 4u,
    BFM_VILLAGE_SHOW_HUD = 8u, BFM_VILLAGE_RELEASE = 16u, BFM_VILLAGE_TRANSFER = 32u
};

typedef struct {
    BfmBedroomRoot root;
    BfmBedroomPose pose;
    BfmVillagePhase phase;
    unsigned controlled, events;
    uint32_t message;
    unsigned movement; /* Cable updates since mounting, retained through impact. */
    uint16_t destination, wall_contact;
    uint8_t entrance;
} BfmVillagePlayer;

/* Borrow the unpacked SC03/2 room and the resident MAIN/10 type-0x101 payload. */
BfmVillageMotion *bfm_village_motion_open(const uint8_t *scene, size_t scene_bytes,
    const uint8_t *main, size_t main_bytes, uint32_t room, unsigned entrance);
/* The marked floor queues a descriptor. The destination restores integer XYZ
 * and heading captured before the final movement pass in the departing field. */
const uint8_t *bfm_village_motion_transfer(const BfmVillageMotion *motion);
BfmVillageMotion *bfm_village_motion_woods(const uint8_t *scene, size_t bytes,
                                           const uint8_t *main, size_t main_bytes);
/* Bind the mounted exit trigger for the current woods story visit. */
int bfm_village_motion_trail_gate(BfmVillageMotion *motion, unsigned story);
struct BfmVillageGround;
const struct BfmVillageGround *bfm_village_motion_ground(const BfmVillageMotion *motion);
BfmVillageMotion *bfm_village_motion_arrive(const BfmVillageMotion *departed, const uint8_t *scene,
                                            size_t bytes, uint32_t room_address);
BfmVillageMotion *bfm_village_motion_resume(const uint8_t *scene, size_t bytes,
                                            uint32_t room_address,
                                            const uint8_t *main, size_t main_bytes);
/* Submit one scripted root and advance the player model once, then use place
 * on release to resume ordinary movement from the script endpoint. */
int bfm_village_motion_porch(BfmVillageMotion *motion, const BfmBedroomRoot *root, int knock);
int bfm_village_motion_staged(BfmVillageMotion *motion, const BfmBedroomRoot *root);
int bfm_village_motion_place(BfmVillageMotion *motion, const BfmBedroomRoot *root);
const BfmVillagePlayer *bfm_village_motion_player(const BfmVillageMotion *motion);
/* Once per gameplay update. Pad axes are -127..127, right and down positive.
 * Camera yaw is target minus eye in X/Z, with +Z at zero. */
int bfm_village_motion_step(BfmVillageMotion *motion, int stick_x, int stick_y, int camera_yaw);
struct BfmGameplayRun;
int bfm_village_motion_step_field(BfmVillageMotion *motion, int stick_x, int stick_y,
                                  int camera_yaw, struct BfmGameplayRun *run);
struct BfmWoodsLilies;
int bfm_village_motion_step_woods(BfmVillageMotion *motion, int stick_x, int stick_y,
                                 int camera_yaw, int cross_pressed, int cross_held,
                                 int actors_running, struct BfmWoodsLilies *lilies);
/* The room owns competing actions such as a cast or guard. Ordinary grounded
 * movement decides whether the standing action actually ran this field. */
int bfm_village_motion_step_woods_field(BfmVillageMotion *motion, int stick_x, int stick_y,
    int camera_yaw, int cross_pressed, int cross_held, int actors_running,
    struct BfmWoodsLilies *lilies, struct BfmGameplayRun *run, int idle_allowed);
/* During Leno's greeting, replace the ordinary woods step with one stationary
 * turn or gesture update. End when the dog returns control after its last page. */
int bfm_village_motion_leno_greeting(BfmVillageMotion *motion, struct BfmWoodsLilies *lilies,
    int actors_running, const int16_t target[3], int speaking);
int bfm_village_motion_end_leno_greeting(BfmVillageMotion *motion);
struct BfmWoodsCarryFrame;
/* Replaces the ordinary woods step for one lift/overhead/throw field. */
int bfm_village_motion_carry(BfmVillageMotion *motion, int stick_x, int stick_y, int camera_yaw,
                             int actors_running, struct BfmWoodsLilies *lilies,
                             const struct BfmWoodsCarryFrame *frame);
/* Apply the push pose after the existing ordinary walking and contact pass. */
int bfm_village_motion_brace(BfmVillageMotion *motion, const struct BfmWoodsCarryFrame *frame);

typedef enum {
    BFM_WOODS_HURT_START, BFM_WOODS_HURT_HOLD, BFM_WOODS_GUARD_RECOIL
} BfmWoodsReactionMotion;

typedef struct {
    int lost_support, landed;
    uint16_t slope, material;
} BfmWoodsMotionContact;

/* Replace step_woods for one reaction field. A hurt holds the player's
 * root. Guard recoil supplies a signed world X/Z displacement in Q16.
 * Platforms advance once; solid actors use the resulting foot segment.
 * Slope and material describe the swept floor. An ordinary wall alone
 * changes neither support edge. */
int bfm_village_motion_woods_reaction(BfmVillageMotion *motion, struct BfmWoodsLilies *lilies,
    BfmWoodsReactionMotion kind, int32_t dx_q16, int32_t dz_q16, BfmWoodsMotionContact *contact);
/* Carry the planted hero with the platform and classify the support beneath a
 * grounded sword cut. A missing or steep support selects fall or slide at once.
 * The finishing field returns to ordinary stance without another support test. */
int bfm_village_motion_woods_cut(BfmVillageMotion *motion, struct BfmWoodsLilies *lilies,
                                unsigned cancel_countdown, int finishing, int *supported);
int bfm_village_motion_ability_pose(BfmVillageMotion *motion, unsigned face_slot, int finished);
int bfm_village_motion_combat_face(BfmVillageMotion *motion, unsigned slot);
/* On a surviving ordinary hurt, select idle or the first fall pose at the
 * current root. The next ordinary field samples input and integrates gravity. */
int bfm_village_motion_woods_recover(BfmVillageMotion *motion, int resume_fall);
/* Recheck actor and stationary terrain support at the current feet after a
 * stocked recovery. The normal field step still consumes its neutral pad. */
int bfm_village_motion_woods_revive(BfmVillageMotion *motion, int tired);
void bfm_village_motion_woods_condition(BfmVillageMotion *motion, int tired);
int bfm_village_motion_woods_sliding(const BfmVillageMotion *motion);
int bfm_village_motion_grounded(const BfmVillageMotion *motion);
/* Read after the ordinary woods step, using the world field that ran it. */
BfmForestDustEmission bfm_village_motion_foot_dust(const BfmVillageMotion *motion,
                                                   unsigned actor_tick);
struct BfmVillageResidents;
/* Resolve contact against the actor transform from the previous field update. */
int bfm_village_motion_correct_contact(BfmVillageMotion *motion, const int16_t xyz[3]);
int bfm_village_motion_resident_contact(BfmVillageMotion *motion,
    const struct BfmVillageResidents *residents, unsigned field_tick, const int16_t previous[3]);
/* Signal music readiness or closure of the current message before the next step. */
int bfm_village_motion_continue(BfmVillageMotion *motion);
/* Start only after the resident has accepted Talk. The mounted conversation
 * specifies the walking offset in the resident's local coordinates. */
int bfm_village_motion_begin_talk(BfmVillageMotion *motion, const int16_t resident_xyz[3],
                                  unsigned resident_yaw);
/* Pass the previous view's eye and target while its conversation zoom moves. */
int bfm_village_motion_view_yaw(const int32_t eye[3], const int32_t target[3]);
int bfm_village_motion_begin_read(BfmVillageMotion *motion, const int16_t point[3]);
int bfm_village_motion_end_read(BfmVillageMotion *motion);
int bfm_village_motion_talk_ready(const BfmVillageMotion *motion);
int bfm_village_motion_end_talk(BfmVillageMotion *motion);
int bfm_village_motion_face(const BfmVillageMotion *motion, uint16_t tiles[512][1024]);
int bfm_village_motion_lower_orb(const BfmVillageMotion *motion, unsigned orb,
                                uint16_t tiles[512][1024]);
void bfm_village_motion_free(BfmVillageMotion *motion);

#endif
