/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_LARDWICK_FIELD_H
#define BFM_WOODS_LARDWICK_FIELD_H

#include "bedroom_travel.h"
#include "forest_guard_contact.h"
#include "forest_message.h"
#include "gameplay_run.h"
#include "woods_lardwick_award.h"
#include "woods_lardwick_motes.h"

typedef enum {
    BFM_LARDWICK_WINDOW_NONE, BFM_LARDWICK_WINDOW_SOLDIER, BFM_LARDWICK_WINDOW_AWARD
} BfmLardwickWindow;

typedef struct {
    BfmBedroomRoot root;
    unsigned light_pitch, light_yaw, detector_step;
    int active, lights_active, flash;
    int glow_visible, crystal_visible, soldier_visible;
    unsigned glow_scale, burst_updates, soldier_yaw, soldier_pose_age;
    uint32_t soldier_model, soldier_animation, soldier_talk_animation;
    BfmForestGuardBox bounds;
    BfmForestTransform glow_transform;
    const uint8_t *accepted_hits, *scene, *script, *award_script;
    size_t bytes;
    unsigned phase, wait;
    BfmLardwickWindow window;
    BfmWoodsLardwickAward award;
    BfmWoodsLardwickMotes motes;
} BfmWoodsLardwickField;

typedef struct {
    unsigned sound;
    /* A nonzero camera address selects the mounted camera relative to the Soldier. */
    uint32_t camera;
    int begin_approach, camera_release, depart, award_begin;
    /* The approach displacement is in the Soldier's local frame. */
    int16_t approach_x, approach_z;
} BfmWoodsLardwickEvents;

/* Mount the captive and his authored rescue script from SC03/29. A saved
 * rescue suppresses the actor on subsequent room entries. */
int bfm_woods_lardwick_field_open(BfmWoodsLardwickField *field, const uint8_t *scene,
                                 size_t bytes, const uint8_t variables[256]);
void bfm_woods_lardwick_field_step(BfmWoodsLardwickField *field, BfmGameplayRun *run,
                                  const int16_t player[3], unsigned field_tick);

/* Check a wall-clipped blade segment against the current crystal transform.
 * Commit the event in actor-slot order and shorten the chord at each contact. */
int bfm_woods_lardwick_contact(const BfmWoodsLardwickField *field, unsigned event,
                               const int16_t from[3], const int16_t to[3], int16_t point[3],
                               int16_t normal[3]);
int bfm_woods_lardwick_strike(BfmWoodsLardwickField *field, unsigned event);
int bfm_woods_lardwick_busy(const BfmWoodsLardwickField *field);
/* Registration unlocks the player's controls and world clock. The invisible
 * actor still owns one cleanup update after the count is published. */
int bfm_woods_lardwick_controls_locked(const BfmWoodsLardwickField *field);

/* One eligible actor update following the strike. The field presentation owns
 * any returned window until its ordinary close callback. The camera
 * eye determines the radial orientation at birth and the Soldier facing at spawn.
 * Supply camera_idle from the camera state before its current field update. */
int bfm_woods_lardwick_release_step(BfmWoodsLardwickField *field, BfmGameplayRun *run,
                                   BfmSaveRecord *save, const uint8_t *main, size_t main_bytes,
                                   const int32_t camera_eye[3], int player_queue_idle, int camera_idle,
                                   BfmForestMessage **opened, BfmWoodsLardwickEvents *events);
BfmForestMessage *bfm_woods_lardwick_notice(BfmWoodsLardwickField *field);
int bfm_woods_lardwick_close(BfmWoodsLardwickField *field);

#endif
