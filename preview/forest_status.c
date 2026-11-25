/* SPDX-License-Identifier: MIT */
#include "forest_status.h"
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
