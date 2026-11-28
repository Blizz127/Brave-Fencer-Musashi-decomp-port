/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_PIT_H
#define BFM_VILLAGE_MINE_PIT_H

#include "forest_bank_thorn_hurt.h"

typedef struct BfmVillageMinePit {
    BfmForestBankThornHurt recovery;
} BfmVillageMinePit;

/* Update after ordinary grounded movement. Only the room's tested solid
 * footing can replace the rescue point. Riding a deck alone cannot. */
void bfm_village_mine_pit_remember(BfmVillageMinePit *pit, const BfmVillageGround *ground,
                                   const int16_t foot[3]);

/* Step before ordinary control. Supply the prior movement's actual mounted
 * contact material and current vertical velocity, even on a terrain miss.
 * An accepted fall releases the rider. The player becomes hidden at the end
 * of the descent and takes damage upon returning to qualified ground.
 * The landing frame reports idle action 0, phase 1, with zero vertical speed. */
int bfm_village_mine_pit_step(BfmVillageMinePit *pit, BfmGameplayRun *run, uint16_t material,
    int32_t vertical_velocity_q16, const BfmForestBankThornInput *input, BfmForestBankThornFrame *frame);

#endif
