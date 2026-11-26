/* SPDX-License-Identifier: MIT */
#include "forest_fusion_power.h"
#include "bedroom_score.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"

int bfm_forest_fusion_power_load(const MusashiDiscMedia *disc, const BfmForestField *field,
                                BfmForestFusionPower *out)
{
    uint8_t ranks[30 * 16];
    if (!disc || !field || !out ||
        !bfm_bedroom_resident_data(disc, UINT32_C(0x800638f8), ranks, sizeof ranks)) return 0;

    BfmForestFusionPower loaded;
    for (unsigned i = 0; i < 30; ++i) {
        loaded.body[i] = u16le(ranks + i * 16 + 2);
        loaded.sword[i] = u16le(ranks + i * 16 + 14);
    }
    const uint8_t *moves = bfm_forest_room_bytes(field, UINT32_C(0x801889dc), 40);
    if (!moves) return 0;
    for (unsigned i = 0; i < 2; ++i) {
        const uint8_t *entry = moves + 20 * i;
        uint16_t kind = u16le(entry);
        const uint8_t *row = bfm_forest_room_bytes(field, UINT32_C(0x80188b80) + 12u * kind, 12);
        /* These two grounded Fusion moves have one scaled body/sword term.
         * More complex attacks use their own stat or target-dependent path. */
        if (!row || !kind || u32le(entry + 4) || (u16le(row) & 0xf000) != 0x4000 ||
            u16le(row + 4) || u16le(row + 6)) return 0;
        loaded.move[i].percent = u16le(row) & 0xfff;
        loaded.move[i].spread = u16le(row + 2);
    }
    *out = loaded;
    return 1;
}

int bfm_forest_fusion_strength(const BfmForestFusionPower *power, const BfmForestStatus *status,
                              unsigned move, int boosted, unsigned random_sample, uint16_t *strength)
{
    if (!power || !status || !strength || move >= 2 || random_sample > 32767) return 0;
    uint32_t body = u32le(status->record + 0x6c), sword = u32le(status->record + 0x7c);
    if (body >= 30 || sword >= 30) return 0;
    unsigned base = (power->body[body] + power->sword[sword]) * (boosted ? 150u : 125u) / 100u;
    unsigned spread = power->move[move].spread;
    unsigned percent = power->move[move].percent + (spread ? random_sample % spread : 0);
    uint64_t amount = (uint64_t)base * percent / 100u;
    if (amount > UINT16_MAX) return 0;
    *strength = (uint16_t)amount;
    return 1;
}
