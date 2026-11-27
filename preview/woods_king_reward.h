/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_KING_REWARD_H
#define BFM_WOODS_KING_REWARD_H

#include "bedroom_record.h"

typedef struct {
    uint8_t ranks[30][16];
    uint16_t figure_price;
    uint8_t figure_trend;
    uint16_t caster_price;
    uint8_t caster_trend;
} BfmWoodsKingReward;

int bfm_woods_king_reward_load(BfmWoodsKingReward *reward, const MusashiDiscMedia *disc,
                               const uint8_t *main, size_t main_bytes);
/* The room issues this once when the King finishes flattening. Return the
 * actual gain in Body for the notice. The shop unlock is independent of rank. */
unsigned bfm_woods_king_reward_apply(const BfmWoodsKingReward *reward, unsigned experience,
                                     BfmSaveRecord *save);


/* A purple caster defeated by a weapon unlocks the next figure and awards
 * its own Body experience through the same rank table as the King. */
unsigned bfm_woods_caster_reward_apply(const BfmWoodsKingReward *reward, unsigned experience,
                                       BfmSaveRecord *save);
#endif
