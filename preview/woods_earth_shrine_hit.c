/* SPDX-License-Identifier: MIT */
#include "woods_earth_shrine_hit.h"
#include "../pc_port/forest_player_bytes.h"
#include <string.h>

static const uint8_t *scene_span(const uint8_t *scene, size_t bytes, uint32_t address, size_t count)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (address < base) return NULL;
    size_t offset = address - base;
    return offset <= bytes && count <= bytes - offset ? scene + offset : NULL;
}

int bfm_woods_earth_shrine_hit_mount(const uint8_t *scene, size_t bytes,
                                    BfmWoodsEarthShrineHit *out)
{
    if (!scene || !out) return 0;
    const uint8_t *placements = scene_span(scene, bytes, UINT32_C(0x8019629c), 8);
    const uint8_t *collider = scene_span(scene, bytes, UINT32_C(0x80195998), 16);
    BfmWoodsEarthShrineHit mounted = {0};
    const uint8_t *classes = scene_span(scene, bytes, UINT32_C(0x80195cf0), sizeof mounted.classes);
    if (!placements || !collider || !classes || u32le(collider) != 0) return 0;

    uint32_t start = u32le(placements), end = u32le(placements + 4);
    if (end <= start || (end - start) % 20) return 0;
    size_t size = end - start;
    const uint8_t *rows = scene_span(scene, bytes, start, size);
    if (!rows) return 0;
    for (size_t offset = 0; offset < size; offset += 20) {
        const uint8_t *row = rows + offset;
        unsigned type = u16le(row + 6);
        if (!type) break;
        if (type != 268 || u16le(row + 8) != 0) continue;
        int16_t position[3];
        for (unsigned axis = 0; axis < 3; ++axis) {
            position[axis] = s16le(row + 2 * axis);
            mounted.bounds.low[axis] = s16le(collider + 4 + 4 * axis);
            mounted.bounds.high[axis] = s16le(collider + 6 + 4 * axis);
            if (mounted.bounds.low[axis] > mounted.bounds.high[axis]) return 0;
        }
        bfm_forest_actor_root(u16le(row + 12) & 4095, position, &mounted.root);
        memcpy(mounted.classes, classes, sizeof mounted.classes);
        mounted.armed = 1;
        *out = mounted;
        return 1;
    }
    return 0;
}

BfmEarthShrineHitResult bfm_woods_earth_shrine_hit_step(BfmWoodsEarthShrineHit *receiver,
                                                       const BfmVillageGround *ground,
                                                       const BfmWoodsCasterMeleeFrame *frame)
{
    if (!receiver || !receiver->armed || !ground || !frame || !frame->event ||
        frame->rays > sizeof frame->to / sizeof frame->to[0]) return BFM_EARTH_SHRINE_MISS;
    for (unsigned ray = 0; ray < frame->rays; ++ray) {
        int16_t center[3], end[3], normal[3];
        uint16_t material;
        memcpy(end, frame->to[ray], sizeof end);
        bfm_village_ground_blade(ground, frame->from, end, normal, &material);
        if (!bfm_forest_guard_contact(&receiver->bounds, &receiver->root,
                                      frame->from, end, center)) continue;
        for (size_t i = 0; i < sizeof receiver->classes; ++i) {
            if (frame->event == receiver->classes[i]) {
                receiver->armed = 0;
                return BFM_EARTH_SHRINE_ACCEPTED;
            }
        }
        return BFM_EARTH_SHRINE_DEFLECTED;
    }
    return BFM_EARTH_SHRINE_MISS;
}
