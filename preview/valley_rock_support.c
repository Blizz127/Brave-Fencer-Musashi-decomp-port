/* SPDX-License-Identifier: MIT */
#include "valley_rock_support.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t length)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (address < base || address - base > bytes || length > bytes - (address - base)) return NULL;
    return scene + (address - base);
}

int bfm_valley_rock_support_load(BfmValleyRockSupport *support, const uint8_t *scene, size_t bytes)
{
    if (!support || !scene) return 0;
    const uint8_t *profiles = mounted(scene, bytes, UINT32_C(0x80186000), 2 * 52);
    if (!profiles) return 0;
    BfmValleyRockSupport parsed;
    for (unsigned i = 0; i < 2; ++i) {
        const uint8_t *shape = mounted(scene, bytes, u32le(profiles + 52 * i + 8), 16);
        if (!shape || u32le(shape)) return 0;
        for (unsigned axis = 0; axis < 3; ++axis) {
            parsed.box[i].low[axis] = s16le(shape + 4 + 4 * axis);
            parsed.box[i].high[axis] = s16le(shape + 6 + 4 * axis);
            if (parsed.box[i].low[axis] >= parsed.box[i].high[axis]) return 0;
        }
    }
    *support = parsed;
    return 1;
}

int bfm_valley_rock_support_probe(const BfmValleyRockSupport *support, unsigned model,
                                  const BfmForestTransform *root, BfmValleyRockProbe kind,
                                  const int16_t from[3], const int16_t to[3],
                                  BfmForestContact *surface)
{
    if (!support || model >= 2 || !root || !from || !to || !surface) return 0;
    int16_t start[3] = {from[0], from[1], from[2]};
    int16_t end[3] = {to[0], to[1], to[2]};
    switch (kind) {
    case BFM_VALLEY_ROCK_LANDING:
        if (end[1] <= start[1]) return 0;
        break;
    case BFM_VALLEY_ROCK_PLANTED:
        if (end[1] < INT16_MIN + 3 || end[1] > INT16_MAX - 5) return 0;
        start[0] = end[0];
        start[1] = end[1] - 3;
        start[2] = end[2];
        end[1] += 5;
        break;
    case BFM_VALLEY_ROCK_SHADOW:
        if (root->translation[1] <= start[1] || root->translation[1] > INT16_MAX) return 0;
        end[0] = start[0];
        end[1] = (int16_t)root->translation[1];
        end[2] = start[2];
        break;
    default:
        return 0;
    }

    int16_t center[3], hit[3], normal[3];
    if (!bfm_forest_guard_contact_point(&support->box[model], root, start, end,
                                        center, hit, normal) || normal[1] >= -3019)
        return 0;
    /* The upright and quarter-turned resting boxes both have a horizontal
     * top. The common segment test stops two units outside that face. */
    *surface = (BfmForestContact){.height = hit[1], .normal = {normal[0], normal[1], normal[2]},
                                  .slope = 0x2000};
    if (kind == BFM_VALLEY_ROCK_SHADOW) surface->height += 2;
    return 1;
}
