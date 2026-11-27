/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_REVIVE_H
#define BFM_WOODS_REVIVE_H

#include "bedroom_record.h"
#include "forest_message.h"
#include "gameplay_run.h"

typedef enum BfmWoodsReviveResult {
    BFM_WOODS_REVIVE_ERROR = -1,
    BFM_WOODS_REVIVE_UNAVAILABLE,
    BFM_WOODS_REVIVE_READY
} BfmWoodsReviveResult;

/* On READY the room owns the cursor and the inventory still contains the item. */
enum BfmWoodsReviveResult bfm_woods_revive_prepare(const BfmGameplayRun *run,
    BfmSaveRecord *state, int item_blocked, const uint8_t *main, size_t bytes,
    unsigned *slot, BfmForestMessage **opened);

/* Commit the selected item once the player has successfully reentered the field. */
void bfm_woods_revive_restore(BfmGameplayRun *run, BfmSaveRecord *state, unsigned slot);

#endif
