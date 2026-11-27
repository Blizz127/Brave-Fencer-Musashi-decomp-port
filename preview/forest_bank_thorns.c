/* SPDX-License-Identifier: MIT */
#include "forest_bank_thorns.h"

int bfm_forest_bank_thorn_hit(uint16_t *reason, uint32_t event_state,
                             uint8_t equipped_ability, uint16_t source_yaw,
                             BfmWoodsCasterRingHit *incoming)
{
    if (*reason != BFM_FOREST_BANK_THORN_REASON)
        return 0;
    *reason = 0;
    if ((event_state & UINT32_C(0x80ffffff)) || equipped_ability == 12) return 0;
    incoming->flags = 0xc007;
    incoming->power = 32;
    incoming->yaw = source_yaw;
    return 1;
}
