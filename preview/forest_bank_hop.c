/* SPDX-License-Identifier: MIT */
#include "forest_bank_hop.h"
#include "field_waypoint.h"
#include "../pc_port/forest_player_bytes.h"
#include <string.h>

enum { HOP = 12, Q16 = 65536 };

int bfm_forest_bank_hop_init(BfmForestBankHop *hop, const uint8_t *scene, size_t bytes)
{
    const size_t price = UINT32_C(0x8018f2c0) - UINT32_C(0x80128158) + HOP;
    if (!hop || !scene || bytes <= price || !(scene[price] & 0x80)) return 0;
    *hop = (BfmForestBankHop){.cost = scene[price] & 0x7f};
    return 1;
}

void bfm_forest_bank_hop_rebound(BfmForestBankHop *hop)
{
    hop->vertical_q16 = -24 * Q16;
}

int16_t bfm_forest_bank_hop_camera_y(const BfmForestBankHop *hop, int16_t player_y)
{
    return hop->active && abs(player_y - hop->launch_y) <= 128 ? hop->launch_y : player_y;
}

BfmBankHopFrame bfm_forest_bank_hop_step(BfmForestBankHop *hop, BfmForestStatus *player,
                                         int x, int y, int camera_yaw, unsigned facing,
                                         int circle_pressed)
{
    BfmBankHopFrame frame = {.yaw = facing};
    uint8_t *record = player->record;
    uint8_t *timer = record + 0x50;
    uint32_t remaining = u32le(timer);
    if (record[0x49] != 0) hop->cancelled_timer = 0;
    if (remaining && (record[0x49] == HOP || hop->cancelled_timer)) {
        --remaining;
        for (unsigned i = 0; i < 4; ++i) timer[i] = (uint8_t)(remaining >> (8*i));
    }
    if (record[0x49] != HOP) {
        hop->active = 0;
        if (!remaining) hop->cancelled_timer = 0;
        return frame;
    }
    if (!hop->active) {
        hop->active = 1;
        if (!record[0x4c]) {
            record[0x4c] = 1;
            bfm_forest_status_spend_bp(player, hop->cost);
        }
        bfm_forest_bank_hop_rebound(hop);
        frame.started = 1;
    }
    if (circle_pressed || !remaining) {
        memset(record + 0x49, 0, 4);
        hop->active = 0;
        hop->cancelled_timer = remaining != 0;
        frame.action = BFM_BANK_HOP_CANCEL;
        return frame;
    }
    if (x || y) {
        frame.yaw = (unsigned)(camera_yaw + bfm_waypoint_heading(-x, y)) & 4095;
        frame.speed_q16 = 8 * Q16;
    }
    if (hop->vertical_q16 < 31 * Q16) hop->vertical_q16 += 3 * Q16;
    frame.vertical_q16 = hop->vertical_q16;
    frame.action = BFM_BANK_HOP_MOVE;
    return frame;
}
