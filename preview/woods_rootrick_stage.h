/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_ROOTRICK_STAGE_H
#define BFM_WOODS_ROOTRICK_STAGE_H

#include "woods_rootrick_trigger.h"

typedef enum {
    BFM_ROOTRICK_OFF_FIELD,
    BFM_ROOTRICK_LOAD_CAST,
    BFM_ROOTRICK_BIND_CAST,
    BFM_ROOTRICK_POSED,
    BFM_ROOTRICK_IN_SCRIPT,
    BFM_ROOTRICK_CAST_ERROR
} BfmRootrickStagePhase;

typedef enum {
    BFM_ROOTRICK_RESOURCE_FAILED = -1,
    BFM_ROOTRICK_RESOURCE_PENDING,
    BFM_ROOTRICK_RESOURCE_READY
} BfmRootrickReadiness;

typedef enum {
    BFM_ROOTRICK_NO_CHANGE,
    BFM_ROOTRICK_FADE_SCORE,
    BFM_ROOTRICK_SHOW_IDLE,
    BFM_ROOTRICK_START_OPENING,
    BFM_ROOTRICK_RESOURCE_ERROR
} BfmRootrickStageEvent;

enum {
    BFM_ROOTRICK_CAST_SC02_MEMBER = 12,
    BFM_ROOTRICK_OPENING_SCORE = 38,
    BFM_ROOTRICK_RACE_SCORE = 28
};
#define BFM_ROOTRICK_STANDING_CLIP UINT32_C(0x801f3ea4)

typedef struct {
    unsigned selector;
    unsigned remaining_updates;
    enum { BFM_ROOTRICK_SCORE_QUIET, BFM_ROOTRICK_SCORE_FADING,
           BFM_ROOTRICK_SCORE_REQUESTED } state;
} BfmRootrickScore;

typedef struct {
    BfmRootrickStagePhase phase;
    int16_t actor_xyz[3]; /* The mounted kind-128 spawn, initially upright at yaw zero. */
    uint32_t opening_address;
    BfmRootrickScore score;
} BfmWoodsRootrickStage;

/* Zero-initialize once per room visit. Call after ordinary movement/time when
 * the mounted bend trigger fires. On success the caller takes control and
 * activates the arena walls. On following updates the staged player cannot
 * move freely or accrue normal field time. The existing HUD is still drawn. */
int bfm_woods_rootrick_stage_begin(BfmWoodsRootrickStage *stage, const BfmWoodsRootrickTrigger *trigger);

/* One ordinary actor update, beginning on the field after stage_begin.
 * LOAD_CAST consumes actual disc readiness. BIND_CAST consumes the result of
 * binding the mounted model. SHOW_IDLE installs standing pose and shadow.
 * START_OPENING transfers the held controls to the VM; run its first commands
 * this same update. The VM owns HUD changes and the later release to play. */
BfmRootrickStageEvent bfm_woods_rootrick_stage_step(BfmWoodsRootrickStage *stage,
                                                    BfmRootrickReadiness cast,
                                                    BfmRootrickReadiness model);

/* Schedule the new score. The caller fades the current one immediately.
 * stage_step handles this scheduling for the opening and returns FADE_SCORE.
 * Later scene cues can supersede it while the encounter keeps moving. */
void bfm_woods_rootrick_score_arm(BfmRootrickScore *score, unsigned selector);

/* Run before stage_step, once per field including during the script.
 * A nonzero result asks the audio backend to request/poll score->selector.
 * Continue until the backend confirms this selector, then call acknowledge. */
int bfm_woods_rootrick_score_due(BfmRootrickScore *score);
void bfm_woods_rootrick_score_acknowledge(BfmRootrickScore *score, unsigned selector);

#endif
