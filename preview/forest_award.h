/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_AWARD_H
#define BFM_FOREST_AWARD_H

#include "forest_cast.h"
#include "forest_absorption.h"

typedef enum {
    BFM_AWARD_IDLE, BFM_AWARD_CATCH, BFM_AWARD_LOADING, BFM_AWARD_OPEN,
    BFM_AWARD_WAIT, BFM_AWARD_CLEAR, BFM_AWARD_RELEASE, BFM_AWARD_COMPLETE, BFM_AWARD_ERROR
} BfmForestAwardPhase;

typedef struct {
    BfmForestAwardPhase phase;
    const MusashiDiscMedia *disc;
    size_t offset, bytes, loaded;
    unsigned key;
    uint32_t pose, visible;
    /* The equipped skill's resource remains available for its native owner
     * after the announcement. Only the first Gunshot package is supported. */
    uint8_t package[64 * 1024];
} BfmForestAward;

/* Invoke at the catch of a successfully absorbed returning blade. The
 * pose owner and disc must remain alive throughout this transaction. */
int bfm_forest_award_begin(BfmForestAward *award, const MusashiDiscMedia *disc,
    const BfmForestCast *cast, const BfmForestTether *tether, BfmForestStatus *status);

/* OPEN requests presentation of tether->award_message once. Supply closed
 * only after the presenter finishes. COMPLETE publishes the ability to the
 * resident record on the final release update and hands control back. */
BfmForestAwardPhase bfm_forest_award_tick(BfmForestAward *award, const BfmForestCast *cast,
    BfmForestTether *tether, BfmForestStatus *status, int closed);
#endif
