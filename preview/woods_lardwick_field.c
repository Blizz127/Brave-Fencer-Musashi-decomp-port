/* SPDX-License-Identifier: MIT */
#include "woods_lardwick_field.h"
#include "../pc_port/forest_player_bytes.h"

#include <limits.h>

static const uint32_t scene_base = UINT32_C(0x80128158);
static const uint32_t room = UINT32_C(0x8018ea38);

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t count)
{
    if (address < scene_base) return NULL;
    size_t offset = address - scene_base;
    return offset <= bytes && count <= bytes - offset ? scene + offset : NULL;
}

int bfm_woods_lardwick_field_open(BfmWoodsLardwickField *field, const uint8_t *scene,
                                 size_t bytes, const uint8_t variables[256])
{
    if (!field || !scene || !variables) return 0;
    const uint8_t *header = mounted(scene, bytes, room, 0xbc);
    if (!header) return 0;
    uint32_t first = u32le(header + 0xb4), end = u32le(header + 0xb8);
    if (end < first || (end - first) % 20) return 0;
    const uint8_t *rows = mounted(scene, bytes, first, end - first);
    if (!rows) return 0;

    BfmWoodsLardwickField result = {0};
    int found = 0;
    for (size_t i = 0; i < end - first; i += 20) {
        const uint8_t *row = rows + i;
        unsigned type = u16le(row + 6);
        if (!type) {
            if (!found) return 0;
            *field = result;
            return 1;
        }
        if (type != 296) continue;
        const uint8_t *captive = mounted(scene, bytes, u32le(row + 16), 16);
        if (!captive) return 0;
        if (captive[0] != 11) continue;
        if (found) return 0;
        found = 1;
        if (variables[58 + captive[0]]) continue;
        result.active = 1;
        for (unsigned axis = 0; axis < 3; ++axis) result.root.xyz[axis] = s16le(row + 2 * axis);
        if (result.root.xyz[1] < INT16_MIN + 4) return 0;
        result.root.xyz[1] -= 4;
        result.root.yaw = u16le(row + 12) & 4095u;
        result.root.normal[1] = -4096;
    }
    return 0;
}

static unsigned proximity_step(const int16_t from[3], const int16_t to[3])
{
    int64_t distance2 = 0;
    for (unsigned i = 0; i < 3; ++i) {
        int64_t d = (int)from[i] - to[i];
        distance2 += d * d;
    }
    /* The distance quantizer crosses each 256-unit boundary exactly. */
    for (int band = 0; band < 8; ++band) {
        int radius = (band + 1) * 256;
        if (distance2 < radius * radius) return 68u - 8u * (unsigned)band;
    }
    return 4;
}

void bfm_woods_lardwick_field_step(BfmWoodsLardwickField *field, BfmGameplayRun *run,
                                  const int16_t player[3], unsigned field_tick)
{
    if (!field || !field->active || !run || !player) return;
    if (!(field_tick & 15u)) field->detector_step = proximity_step(field->root.xyz, player);
    field->root.yaw = (field->root.yaw + 32) & 4095u;
    field->flash = (bfm_gameplay_random(run) & 15u) == 0;
    field->lights_active = 1;
    field->light_pitch = (field->light_pitch + 128) & 4095u;
    field->light_yaw = (field->light_yaw + 256) & 4095u;
}
