/* SPDX-License-Identifier: MIT */
#include "forest_status.h"
#include "bedroom_score.h"
#include <string.h>

static uint16_t read16(const uint8_t *p)
{
    return (uint16_t)(p[0] | (unsigned)p[1] << 8);
}

static uint32_t read32(const uint8_t *p)
{
    return read16(p) | (uint32_t)read16(p + 2) << 16;
}

static int16_t signed16(const uint8_t *p)
{
    int32_t value = read16(p);
    return (int16_t)(value > INT16_MAX ? value - 65536 : value);
}

int bfm_forest_hit_damage(const MusashiDiscMedia *disc, const BfmForestStatus *status,
                          uint32_t player_flags, unsigned power, uint16_t *amount)
{
    if (!disc || !status || !amount || !power || power > UINT16_MAX) return 0;
    uint32_t rank = read32(status->record + 0x74);
    uint8_t defense[2];
    if (rank >= 30 || !bfm_bedroom_resident_data(disc, UINT32_C(0x800638fe) + 16 * rank,
                                                 defense, sizeof defense)) return 0;

    unsigned reduction = power * read16(defense) / 80u;
    unsigned damage = reduction >= power ? 1 : power - reduction;
    if (status->record[0x49] == 5 || (player_flags & UINT32_C(0x800))) damage /= 2;
    *amount = (uint16_t)damage;
    return 1;
}

int bfm_forest_rifle_damage(const MusashiDiscMedia *disc, const BfmForestStatus *status,
                            uint32_t player_flags, uint16_t *amount)
{
    return bfm_forest_hit_damage(disc, status, player_flags, 25, amount);
}

void bfm_forest_status_equip_lumina(BfmForestStatus *status)
{
    if (!status) return;
    memcpy(status->record + 0x3c, status->record + 0x3a, 2);
    memcpy(status->record + 0x40, status->record + 0x3e, 2);
    status->record[0x48] |= 1u;
}

void bfm_forest_status_debit(BfmForestStatus *status, unsigned amount)
{
    if (!status) return;
    int hp = signed16(status->record + 0x3c);
    unsigned remaining = hp > 0 && (unsigned)hp > amount ? (unsigned)hp - amount : 0;
    status->record[0x3c] = (uint8_t)remaining;
    status->record[0x3d] = (uint8_t)(remaining >> 8);
}

void bfm_forest_status_spend_bp(BfmForestStatus *status, unsigned amount)
{
    if (!status) return;
    unsigned bp = read16(status->record + 0x40);
    unsigned remaining = bp > amount ? bp - amount : 0;
    status->record[0x40] = (uint8_t)remaining;
    status->record[0x41] = (uint8_t)(remaining >> 8);
}

int bfm_forest_status_load(const MusashiDiscMedia *disc, BfmForestStatus *out)
{
    static const uint32_t template_address = UINT32_C(0x80072c84);
    uint8_t header[32];
    size_t size;
    if (!disc || !out || !musashi_disc_media_read_root_range(
            disc, "SLUS_007.26", 0, header, sizeof(header), &size) ||
        memcmp(header, "PS-X EXE", 8) || size < 2048)
        return 0;

    uint32_t base = read32(header + 24);
    uint32_t bytes = read32(header + 28);
    if (base > template_address || bytes > size - 2048 || bytes < sizeof(out->record))
        return 0;
    uint32_t offset = template_address - base;
    BfmForestStatus value;
    if (offset > bytes - sizeof(value.record) || !musashi_disc_media_read_root_range(
            disc, "SLUS_007.26", (size_t)offset + 2048, value.record, sizeof(value.record), &size))
        return 0;

    /* The resident clock stores 16.16 minutes since midnight. Rebuild its
     * display cache as the original reset does. The opening precedes the
     * story flag that enables world time to advance. */
    unsigned minutes = read16(value.record + 0x1a);
    if (minutes >= 24 * 60 || value.record[0x42] >= 7)
        return 0;
    value.record[0x38] = (uint8_t)(minutes % 60);
    value.record[0x39] = (uint8_t)(minutes / 60);
    *out = value;
    return 1;
}

BfmForestStatusValues bfm_forest_status_values(const BfmForestStatus *status)
{
    const uint8_t *p = status->record;
    return (BfmForestStatusValues) {
        .wallet = read32(p + 0x14),
        .minutes_of_day = read16(p + 0x1a),
        .face_gauge = read16(p + 0x2e),
        .day = read16(p + 0x34),
        .max_hp = read16(p + 0x3a),
        .hp = signed16(p + 0x3c),
        .max_bp = read16(p + 0x3e),
        .bp = signed16(p + 0x40),
        .weekday = p[0x42],
        .charge = p[0x47],
        .orb = p[0x48],
        .ability = p[0x49] & 0x7f,
        .ability_label = p[0x4a] & 0x7f
    };
}
