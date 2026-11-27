/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_LARDWICK_AWARD_H
#define BFM_WOODS_LARDWICK_AWARD_H

#include "forest_message.h"
#include "gameplay_run.h"
#include "village_arrival_motion.h"

typedef struct {
    const uint8_t *message;
    size_t message_bytes;
    unsigned wait_fields;
    uint16_t figure_price;
    uint8_t figure_trend;
    unsigned player_phase, saved_heading;
    int bp_applied, body_ended;
} BfmWoodsLardwickAward;

/* Borrow the common Bincho reward routine reached by the captive's mounted
 * script and its shared player text. Keep both payloads alive for this visit. */
int bfm_woods_lardwick_award_open(BfmWoodsLardwickAward *award,
    const uint8_t *scene, size_t scene_bytes, const uint8_t *main, size_t main_bytes,
    uint32_t common_script);
BfmForestMessage *bfm_woods_lardwick_award_message(const BfmWoodsLardwickAward *award);

typedef struct {
    unsigned sound;
    int open_notice, complete;
} BfmLardwickAwardFrame;

/* The script queues this action after an ordinary player update. Step it once
 * on each subsequent player update, including while its own notice is open.
 * On open_notice the room opens the borrowed message; window_closed is its
 * final closing edge. At complete the actor releases the queued player order. */
int bfm_woods_lardwick_award_begin(BfmWoodsLardwickAward *award, const BfmVillageMotion *motion);
int bfm_woods_lardwick_award_step(BfmWoodsLardwickAward *award, BfmVillageMotion *motion,
    const BfmBedroomView *view, int window_closed, BfmLardwickAwardFrame *out);

/* The script grants BP after its mounted delay. Return its single sound cue. */
unsigned bfm_woods_lardwick_award_bp(BfmWoodsLardwickAward *award,
    BfmSaveRecord *save, BfmGameplayRun *run);
/* The actor records the rescue after the conversation and player award finish.
 * It can then be removed. A previously registered rescue also returns success. */
int bfm_woods_lardwick_award_commit(const BfmWoodsLardwickAward *award,
    BfmSaveRecord *save, BfmGameplayRun *run);

#endif
