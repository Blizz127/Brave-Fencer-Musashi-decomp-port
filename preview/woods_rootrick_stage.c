/* SPDX-License-Identifier: MIT */
#include "woods_rootrick_stage.h"
#include <string.h>

int bfm_woods_rootrick_stage_begin(BfmWoodsRootrickStage *stage, const BfmWoodsRootrickTrigger *trigger)
{
    if (!stage || !trigger || stage->phase != BFM_ROOTRICK_OFF_FIELD ||
        trigger->state != BFM_ROOTRICK_STARTED || !trigger->opening_address) return 0;
    memcpy(stage->actor_xyz, trigger->actor_xyz, sizeof(stage->actor_xyz));
    stage->opening_address = trigger->opening_address;
    stage->phase = BFM_ROOTRICK_LOAD_CAST;
    return 1;
}

BfmRootrickStageEvent bfm_woods_rootrick_stage_step(BfmWoodsRootrickStage *stage,
                                                    BfmRootrickReadiness cast,
                                                    BfmRootrickReadiness model)
{
    if (!stage) return BFM_ROOTRICK_NO_CHANGE;
    switch (stage->phase) {
    case BFM_ROOTRICK_LOAD_CAST:
        if (cast == BFM_ROOTRICK_RESOURCE_FAILED) break;
        if (cast != BFM_ROOTRICK_RESOURCE_READY) return BFM_ROOTRICK_NO_CHANGE;
        bfm_woods_rootrick_score_arm(&stage->score, BFM_ROOTRICK_OPENING_SCORE);
        stage->phase = BFM_ROOTRICK_BIND_CAST;
        return BFM_ROOTRICK_FADE_SCORE;
    case BFM_ROOTRICK_BIND_CAST:
        if (model == BFM_ROOTRICK_RESOURCE_FAILED) break;
        if (model != BFM_ROOTRICK_RESOURCE_READY) return BFM_ROOTRICK_NO_CHANGE;
        stage->phase = BFM_ROOTRICK_POSED;
        return BFM_ROOTRICK_SHOW_IDLE;
    case BFM_ROOTRICK_POSED:
        stage->phase = BFM_ROOTRICK_IN_SCRIPT;
        return BFM_ROOTRICK_START_OPENING;
    default:
        return BFM_ROOTRICK_NO_CHANGE;
    }
    stage->score.state = BFM_ROOTRICK_SCORE_QUIET;
    stage->phase = BFM_ROOTRICK_CAST_ERROR;
    return BFM_ROOTRICK_RESOURCE_ERROR;
}

void bfm_woods_rootrick_score_arm(BfmRootrickScore *score, unsigned selector)
{
    if (!score) return;
    score->selector = selector;
    score->remaining_updates = 30;
    score->state = BFM_ROOTRICK_SCORE_FADING;
}

int bfm_woods_rootrick_score_due(BfmRootrickScore *score)
{
    if (!score || score->state == BFM_ROOTRICK_SCORE_QUIET) return 0;
    if (score->state == BFM_ROOTRICK_SCORE_FADING) {
        if (score->remaining_updates) --score->remaining_updates;
        else score->state = BFM_ROOTRICK_SCORE_REQUESTED;
        return 0;
    }
    return 1;
}

void bfm_woods_rootrick_score_acknowledge(BfmRootrickScore *score, unsigned selector)
{
    if (score && score->state == BFM_ROOTRICK_SCORE_REQUESTED && selector == score->selector)
        score->state = BFM_ROOTRICK_SCORE_QUIET;
}
