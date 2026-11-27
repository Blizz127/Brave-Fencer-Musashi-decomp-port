/* SPDX-License-Identifier: MIT */
#include "woods_revive.h"

#include <string.h>

enum {
    FIRST_ITEM_SLOT = 47, LAST_ITEM_SLOT = 58, REVIVE_ITEM = 10,
    NOTICE_BYTES = 89, NOTICE_ACTIVE = 0x119
};

enum BfmWoodsReviveResult bfm_woods_revive_prepare(const BfmGameplayRun *run,
    BfmSaveRecord *state, int item_blocked, const uint8_t *main, size_t bytes,
    unsigned *slot, BfmForestMessage **opened)
{
    if (!run || !run->has_player || !state || !slot || !opened)
        return BFM_WOODS_REVIVE_ERROR;
    if (item_blocked || bfm_forest_status_values(&run->player).hp != 0)
        return BFM_WOODS_REVIVE_UNAVAILABLE;

    const uint8_t *inventory = run->banks.variables;
    const uint8_t *item = memchr(inventory + FIRST_ITEM_SLOT, REVIVE_ITEM,
                                 LAST_ITEM_SLOT - FIRST_ITEM_SLOT + 1);
    if (!item) return BFM_WOODS_REVIVE_UNAVAILABLE;

    /* The field receives MAIN's expanded player payload. Zero bytes inside the
     * dialogue are operands, so the mounted extent includes the complete close. */
    const size_t offset = 0x80110ec0u - 0x800cedf8u;
    if (*opened || !main || bytes < offset + NOTICE_BYTES ||
        main[offset] != 0x05 || main[offset + NOTICE_BYTES - 1] != 0)
        return BFM_WOODS_REVIVE_ERROR;
    BfmForestMessage *notice = bfm_field_message_borrow_notice(main + offset, NOTICE_BYTES,
                                                              state->flags, NOTICE_ACTIVE);
    if (!notice) return BFM_WOODS_REVIVE_ERROR;
    *opened = notice;
    *slot = (unsigned)(item - inventory);
    return BFM_WOODS_REVIVE_READY;
}

void bfm_woods_revive_restore(BfmGameplayRun *run, BfmSaveRecord *state, unsigned slot)
{
    if (!run || !state || slot < FIRST_ITEM_SLOT || slot > LAST_ITEM_SLOT ||
        run->banks.variables[slot] != REVIVE_ITEM) return;

    uint8_t *player = run->player.record;
    run->banks.variables[slot] = 0;
    state->variables[slot] = 0;
    memcpy(player + 0x3c, player + 0x3a, 2);
    memcpy(player + 0x40, player + 0x3e, 2);
    memset(player + 0x1c, 0, 4);
    memset(player + 0x54, 0, 4);
    memcpy(state->progress + 0x3c, player + 0x3c, 2);
    memcpy(state->progress + 0x40, player + 0x40, 2);
    memset(state->progress + 0x1c, 0, 4);
    memset(state->progress + 0x54, 0, 4);
}
