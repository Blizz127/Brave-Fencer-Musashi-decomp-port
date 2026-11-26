/* SPDX-License-Identifier: MIT */
#include "forest_guard_clip.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"

int bfm_forest_guard_clip_read(const BfmForestField *field, uint32_t list,
                               unsigned terminator, BfmForestGuardClip *clip)
{
    if (!field || !clip || list > UINT32_MAX - 8 * BFM_GUARD_KEYS) return 0;
    BfmForestGuardClip parsed = {0};
    for (unsigned i = 0; i <= BFM_GUARD_KEYS; ++i) {
        const uint8_t *key = bfm_forest_room_bytes(field, list + 8 * i, 8);
        if (!key) return 0;
        uint32_t tagged = u32le(key), duration = u32le(key + 4);
        if (!tagged) {
            if (!i || duration != terminator) return 0;
            *clip = parsed;
            return 1;
        }
        if (i == BFM_GUARD_KEYS || (tagged >> 24) != 0x81 || duration != 1) return 0;
        parsed.pose[i] = bfm_forest_room_bytes(field, tagged & ~UINT32_C(0x01000000), BFM_GUARD_POSE_BYTES);
        if (!parsed.pose[i]) return 0;
        ++parsed.count;
    }
    return 0;
}
