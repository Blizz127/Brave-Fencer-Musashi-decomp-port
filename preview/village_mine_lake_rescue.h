/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_LAKE_RESCUE_H
#define BFM_VILLAGE_MINE_LAKE_RESCUE_H

#include "bedroom_figure.h"
#include "forest_message.h"
#include "gameplay_run.h"
#include "village_arrival_motion.h"
#include "village_camera.h"

typedef struct BfmMineLakeRescue BfmMineLakeRescue;
typedef enum {
    BFM_LAKE_RESCUE_WINDOW_NONE, BFM_LAKE_RESCUE_WINDOW_RESIDENT, BFM_LAKE_RESCUE_WINDOW_AWARD
} BfmMineLakeRescueWindow;
typedef struct { unsigned player_sound, actor_sound; } BfmMineLakeRescueEvents;

/* The room, player resource, and live save must outlive the controller. */
BfmMineLakeRescue *bfm_mine_lake_rescue_open(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes, BfmSaveRecord *save);
void bfm_mine_lake_rescue_free(BfmMineLakeRescue *r);

/* On the glow's approach edge, after this field's player update. Borrow the
 * allocated model and its data through the return to follow camera. The field
 * can give up drawing the resident once this call succeeds. */
int bfm_mine_lake_rescue_begin(BfmMineLakeRescue *r, const BfmGameplayRun *run,
    unsigned id, const BfmBedroomRoot *root, const MusashiForestPlayerMesh *model,
    const uint8_t *resource, size_t resource_bytes, unsigned pose_tick,
    const BfmVillageCamera *presented);
int bfm_mine_lake_rescue_busy(const BfmMineLakeRescue *r);
int bfm_mine_lake_rescue_drives_player(const BfmMineLakeRescue *r);

/* Step before accepting any new approach edge. The area suppresses ordinary
 * player motion while drives_player is set. A newly opened message is owned
 * by the field presenter; close only on its final page's closing edge. */
int bfm_mine_lake_rescue_step(BfmMineLakeRescue *r, BfmGameplayRun *run,
    BfmVillageMotion *motion, const BfmVillageCamera *presented, const BfmBedroomView *view,
    int player_ready, BfmForestMessage **opened, BfmMineLakeRescueEvents *events);
BfmMineLakeRescueWindow bfm_mine_lake_rescue_window(const BfmMineLakeRescue *r,
    const BfmBedroomRoot **speaker);
int bfm_mine_lake_rescue_close(BfmMineLakeRescue *r);
void bfm_mine_lake_rescue_camera(BfmMineLakeRescue *r, BfmVillageCamera *normal);
int bfm_mine_lake_rescue_add(const BfmMineLakeRescue *r, BfmBedroomFigure *figure,
    const BfmBedroomView *view, unsigned tick);

#endif
