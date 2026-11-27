/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_EARTH_PLAYER_AWARD_H
#define BFM_WOODS_EARTH_PLAYER_AWARD_H

#include "gameplay_run.h"

typedef struct BfmWoodsEarthPlayerAward BfmWoodsEarthPlayerAward;

typedef struct {
    /* Completion of the directed turn toward the presented camera eye,
     * the Earth effect's message callback, and the actual completed window. */
    int facing_camera_done, effect_message_ready, window_closed;
} BfmEarthPlayerAwardInput;

typedef struct {
    /* Pose points at a full mounted player key. It lives until free(). */
    const uint8_t *pose;
    unsigned face_slot;
    int face_changed, show_fusion, show_lumina;
    int face_camera, enter_presentation, restore_presentation;
    /* Create the Earth effect at the hero's (0,-96,0) local offset. */
    int spawn_earth_effect, finish_earth_effect;
    const uint8_t *message;
    size_t message_bytes;
    /* Start the ordinary cue or stop only the named active cue. */
    unsigned sound, stop_sound;
    /* committed pulses once; complete also remains set on subsequent steps. */
    int committed, complete;
} BfmEarthPlayerAwardFrame;

/* Call only when the mounted shrine script dispatches its player award
 * descriptor. Scene bytes must remain valid until the action is freed. */
BfmWoodsEarthPlayerAward *bfm_woods_earth_player_award_open(
    const MusashiDiscMedia *disc, const uint8_t *scene, size_t scene_bytes,
    uint32_t descriptor);
void bfm_woods_earth_player_award_free(BfmWoodsEarthPlayerAward *award);
/* One actual player update, including its following body animation update.
 * The caller keeps the normal foot position, drives the turn and face channel,
 * draws the supplied pose, routes effects/audio and owns the real window. */
int bfm_woods_earth_player_award_step(BfmWoodsEarthPlayerAward *award,
    BfmEarthPlayerAwardInput input, BfmSaveRecord *record, BfmGameplayRun *run,
    BfmEarthPlayerAwardFrame *out);

#endif
