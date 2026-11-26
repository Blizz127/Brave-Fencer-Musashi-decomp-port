/* SPDX-License-Identifier: MIT */
#include "forest_absorption.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"

#include <string.h>

int bfm_forest_tether_begin(BfmForestTether *tether, const BfmForestField *field,
                            const BfmForestTetherTarget *target)
{
    if (!tether || !target || tether->phase == BFM_TETHER_ATTACHED ||
        tether->phase == BFM_TETHER_EARNED || target->slot >= 96 ||
        target->generation > INT16_MAX)
        return 0;
    const uint8_t *placement = bfm_forest_room_bytes(field, UINT32_C(0x8018a868), 20);
    if (!placement || target->kind != 47 || target->kind != u16le(placement + 6) ||
        target->variant != u16le(placement + 8))
        return 0;

    unsigned type = target->variant >> 8;
    if (type >= 8) return 0;
    const uint8_t *profile = bfm_forest_room_bytes(field,
        UINT32_C(0x8018abc0) + 52 * type, 48);
    if (!profile) return 0;
    unsigned ability = u16le(profile + 44);
    if (ability != 8) return 0;
    const uint8_t *gains = bfm_forest_room_bytes(field, UINT32_C(0x80189898), 31);
    const uint8_t *duration = bfm_forest_room_bytes(field,
        UINT32_C(0x8018981c) + 4 * ability, 4);
    const uint8_t *message = bfm_forest_room_bytes(field,
        UINT32_C(0x80189514) + 4 * ability, 4);
    if (!gains || !duration || !message || !gains[ability] || !u32le(message))
        return 0;
    *tether = (BfmForestTether){
        .phase = BFM_TETHER_ATTACHED, .target = *target, .ability = ability,
        .gain = gains[ability], .duration = u32le(duration), .award_message = u32le(message)
    };
    return 1;
}

BfmForestTetherPhase bfm_forest_tether_tick(BfmForestTether *tether,
        const BfmForestTetherTarget *target, unsigned room_tick,
        int square_pressed, int interrupted)
{
    if (!tether) return BFM_TETHER_IDLE;
    if (interrupted && (tether->phase == BFM_TETHER_ATTACHED ||
                        tether->phase == BFM_TETHER_EARNED))
        tether->phase = BFM_TETHER_LOST;
    if (tether->phase != BFM_TETHER_ATTACHED) return tether->phase;
    if (!target || target->slot != tether->target.slot ||
        target->generation != tether->target.generation ||
        target->kind != tether->target.kind || target->variant != tether->target.variant) {
        tether->phase = BFM_TETHER_LOST;
        return tether->phase;
    }

    ++tether->ticks;
    if (!(room_tick & 3u)) {
        unsigned loss = tether->decay_clock >> 8;
        tether->meter = tether->meter > loss ? tether->meter - loss : 0;
    }
    tether->decay_clock += 64;
    if (square_pressed) {
        tether->decay_clock = 0;
        tether->meter += tether->gain;
    }
    if (tether->meter >= 128) {
        tether->meter = 128;
        tether->phase = BFM_TETHER_EARNED;
    } else if (tether->ticks >= 128) {
        tether->phase = BFM_TETHER_LOST;
    }
    return tether->phase;
}

int bfm_forest_tether_commit(BfmForestTether *tether, BfmForestStatus *status)
{
    if (!tether || !status || tether->phase != BFM_TETHER_EARNED) return 0;
    uint8_t *record = status->record;
    if (record[0x4b]) {
        /* Replacing an ability that held a temporary stat snapshot restores
         * the original values before returning control to the player. */
        memcpy(record + 0x1c, record + 0x54, 4);
        memcpy(record + 0x24, record + 0x58, 16);
        record[0x43] = record[0x68];
        record[0x4b] = 0;
    }
    record[0x49] = record[0x4a] = (uint8_t)tether->ability;
    record[0x4c] = 0;
    for (unsigned i = 0; i < 4; ++i)
        record[0x50 + i] = (uint8_t)(tether->duration >> (8 * i));
    tether->phase = BFM_TETHER_COMMITTED;
    return 1;
}
