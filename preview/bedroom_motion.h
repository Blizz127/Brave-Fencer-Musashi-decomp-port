/* SPDX-License-Identifier: MIT */
#ifndef BFM_BEDROOM_MOTION_H
#define BFM_BEDROOM_MOTION_H

#include <stddef.h>
#include <stdint.h>

struct BfmGameplayRun;
typedef struct BfmBedroomMotion BfmBedroomMotion;
typedef enum {
    BFM_BEDROOM_STANCE_INACTIVE,
    BFM_BEDROOM_STANCE_IDLE,
    BFM_BEDROOM_STANCE_SCRIPTED,
    BFM_BEDROOM_STANCE_SHEATHED
} BfmBedroomStance;

typedef enum {
    BFM_BEDROOM_NONE,
    BFM_BEDROOM_LIE,
    BFM_BEDROOM_RISE,
    BFM_BEDROOM_YAWN,
    BFM_BEDROOM_UPRIGHT,
    BFM_BEDROOM_WALK,
    BFM_BEDROOM_FALL,
    BFM_BEDROOM_LAND,
    BFM_BEDROOM_STAND,
    BFM_BEDROOM_REST, BFM_BEDROOM_ENTER,
    BFM_BEDROOM_SEARCH,
    BFM_BEDROOM_SHOW_FUSION,
    BFM_BEDROOM_COLLECT, BFM_BEDROOM_RECEIVE, BFM_BEDROOM_LOWER, BFM_BEDROOM_FAREWELL, BFM_BEDROOM_NEGATIVE_REPLY,
    BFM_BEDROOM_CABLE, BFM_BEDROOM_KNOCK,
    BFM_BEDROOM_NIGHTMARE_SLEEP, BFM_BEDROOM_NIGHTMARE_HOLD,
    BFM_BEDROOM_DRAW_WATER
} BfmBedroomAction;

typedef struct {
    const uint8_t *pose; /* 256 mounted bytes. The part transforms start at +16. */
    unsigned face_word;
    unsigned show_weapon; /* The direct body program requests a held Fusion. */
    unsigned show_lumina; /* Driven independently by the body program. */
    unsigned pitch; /* Model X rotation, in 1/4096 turns. */
    unsigned draw_current; /* The active scripted gesture submits its current pose. */
    unsigned finished; /* Player action completed in this update. */
    unsigned knock_contact; /* One update when the knocking hand strikes the door. */
    unsigned hide_fusion; /* The detached sword is submitted as a separate actor. */
    int16_t lumina_scale_q12; /* Zero uses the ordinary drawn Lumina mesh size. */
    unsigned body_contact; /* Mounted event 8 or 9, or zero on a field without contact. */
} BfmBedroomPose;

/* The expanded room member and MAIN/10 type-0x101 payload (after its header)
 * are borrowed for the lifetime of the evaluator. */
BfmBedroomMotion *bfm_bedroom_motion_open(const uint8_t *scene, size_t scene_bytes,
                                          const uint8_t *main, size_t main_bytes, uint32_t room);
typedef enum { BFM_BODY_PLAYING, BFM_BODY_HOLD, BFM_BODY_END } BfmBodyMarker;
/* A room can select a mounted body program before the ordinary animation
 * pass. A new selection consumes its first key immediately. Repeating the
 * current selection keeps its clock, pose and any outstanding hold. */
int bfm_bedroom_motion_direct(BfmBedroomMotion *motion, uint32_t program);
/* The scripted player selects its new first key after the ordinary body and face pass. */
int bfm_bedroom_motion_step_then_restart(BfmBedroomMotion *motion, uint32_t program,
                                         BfmBedroomPose *frame);
/* Choose a field gait after the shared animation update and publish its entry key. */
int bfm_bedroom_motion_field_gait(BfmBedroomMotion *motion, unsigned slot, int entering,
                                  BfmBedroomPose *frame);
int bfm_bedroom_motion_direct_slot(BfmBedroomMotion *motion, unsigned slot);
/* A looping program can display a key and signal its wrap in the same field. */
int bfm_bedroom_motion_looped(const BfmBedroomMotion *motion);
/* Select the resting body after an END, before this field's face advances. */
int bfm_bedroom_motion_finish_program(BfmBedroomMotion *motion, BfmBedroomPose *frame);
void bfm_bedroom_motion_release_draw(BfmBedroomMotion *motion, BfmBedroomPose *frame);
BfmBodyMarker bfm_bedroom_motion_marker(const BfmBedroomMotion *motion);
/* Release a hold at the program's current interval. The next animation
 * pass advances the restored clock. A terminal marker cannot resume. */
int bfm_bedroom_motion_resume(BfmBedroomMotion *motion);
/* Select the ordinary standing entry before this field's animation pass.
 * While the entry plays, refresh its tired flag before subsequent updates. */
int bfm_bedroom_motion_reenter_stance(BfmBedroomMotion *motion, int tired);
/* Advance the old body and face, then install the grounded interaction
 * stance and leave its neutral face ready for the next field. */
int bfm_bedroom_motion_examine_tick(BfmBedroomMotion *motion, int tired, int show_fusion,
                                    BfmBedroomPose *frame);
/* A field ability selects its face after the shared pre-step, then hands
 * control back to the grounded standing initializer when its body ends. */
int bfm_bedroom_motion_ability_pose(BfmBedroomMotion *motion, unsigned face_slot, int finished,
                                    BfmBedroomPose *frame);
int bfm_bedroom_motion_reenter_fall(BfmBedroomMotion *motion, int moving);
/* Release an external body after the ordinary face's tick. Select the new
 * body's first packet now and leave its facial selection for the next field. */
int bfm_bedroom_motion_leave_hop(BfmBedroomMotion *motion, unsigned support,
                                 int moving, BfmBedroomPose *frame);
/* After a falling field's animation pass, release the ordinary fall hold
 * when the vertical velocity reaches zero or points down. */
void bfm_bedroom_motion_descend(BfmBedroomMotion *motion);
int bfm_bedroom_motion_reenter_slide(BfmBedroomMotion *motion);
void bfm_bedroom_motion_stance_condition(BfmBedroomMotion *motion, int tired);
int bfm_bedroom_motion_step(BfmBedroomMotion *motion, BfmBedroomAction action,
                            BfmBedroomPose *frame);
/* One field update. Supply the shared playthrough when Musashi runs his
 * ordinary standing action. Scripted standing keeps the resting body pose.
 * A frozen player receives no update. */
int bfm_bedroom_motion_step_field(BfmBedroomMotion *motion, BfmBedroomAction action,
                                  struct BfmGameplayRun *run, BfmBedroomStance stance,
                                  BfmBedroomPose *frame);
/* Apply a mounted facial texture and palette when the facial channel requests
 * an upload. Tagged in-VRAM facial moves are returned in frame.face_word. */
/* A room-owned body clip can select or continue the ordinary facial channel.
 * An already-stepped ordinary field only advances a newly selected face. */
int bfm_bedroom_motion_external_face(BfmBedroomMotion *motion, unsigned slot,
                                     int already_stepped, BfmBedroomPose *frame);
int bfm_bedroom_motion_face(const BfmBedroomMotion *motion, uint16_t tiles[512][1024]);
int bfm_bedroom_orb_palette(const BfmBedroomMotion *motion, uint32_t table, unsigned orb, uint16_t tiles[512][1024]);
int bfm_field_blade_palette(const BfmBedroomMotion *motion, uint32_t normal_table, unsigned orb,
                            uint16_t tiles[512][1024]);
int bfm_grillin_blade_palette(const BfmBedroomMotion *motion, unsigned orb, uint16_t tiles[512][1024]);
int bfm_bedroom_motion_upload_palette(const BfmBedroomMotion *motion, uint32_t descriptor,
                                      uint16_t tiles[512][1024]);
/* Publish a newly selected recovery pose with level pitch. Its first key is
 * already decoded, so publishing it consumes no additional animation time. */
void bfm_bedroom_motion_publish_upright(BfmBedroomMotion *motion, BfmBedroomPose *frame);
void bfm_bedroom_motion_free(BfmBedroomMotion *motion);

#endif
