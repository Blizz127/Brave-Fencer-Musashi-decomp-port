/* SPDX-License-Identifier: MIT */
#ifndef BFM_VALLEY_KNITTER_H
#define BFM_VALLEY_KNITTER_H

#include "bedroom_figure.h"
#include "forest_message.h"
#include "gameplay_run.h"
#include "village_arrival_motion.h"
#include "village_camera.h"

typedef struct BfmValleyKnitter BfmValleyKnitter;

/* Borrow the room, player data and live save for this visit to 302b. */
BfmValleyKnitter *bfm_valley_knitter_open(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes, BfmSaveRecord *save);
void bfm_valley_knitter_free(BfmValleyKnitter *knitter);
int bfm_valley_knitter_busy(const BfmValleyKnitter *knitter);
/* Skip ordinary motion only while choreographed orders own the player.
 * Busy fields otherwise run the regular player with neutral controls. */
int bfm_valley_knitter_drives_player(const BfmValleyKnitter *knitter);

typedef enum {
    BFM_KNITTER_WINDOW_NONE, BFM_KNITTER_WINDOW_CAPTIVE, BFM_KNITTER_WINDOW_AWARD
} BfmValleyKnitterWindow;
/* Supply the captive's physical speaker root when the mounted stream speaks.
 * BfmForestMessageFrame.portrait selects its image from the room's portrait table. */
BfmValleyKnitterWindow bfm_valley_knitter_window(const BfmValleyKnitter *knitter,
                                                 const BfmBedroomRoot **speaker);

typedef struct {
    unsigned player_sound, actor_sound;
} BfmValleyKnitterEvents;

/* One actor update after the crystal. On reveal pass its physical root.
 * Approach starts the captive's script after the crystal glow clears.
 * player_ready indicates the preceding player action has finished.
 * On opening a window the field takes ownership of *opened. */
int bfm_valley_knitter_step(BfmValleyKnitter *knitter, BfmGameplayRun *run,
    BfmVillageMotion *motion, const BfmVillageCamera *presented, const BfmBedroomView *view,
    int player_ready, const BfmBedroomRoot *crystal, unsigned captive_yaw, int reveal, int approach,
    BfmForestMessage **opened, BfmValleyKnitterEvents *events);
/* Call once on the focused conversation or award window's final close edge. */
int bfm_valley_knitter_close(BfmValleyKnitter *knitter);
/* After ordinary follow produces the new camera, before submitting it. */
void bfm_valley_knitter_camera(BfmValleyKnitter *knitter, BfmVillageCamera *camera);
int bfm_valley_knitter_add(const BfmValleyKnitter *knitter, BfmBedroomFigure *figure,
                          const BfmBedroomView *view, unsigned field_tick);

#endif
