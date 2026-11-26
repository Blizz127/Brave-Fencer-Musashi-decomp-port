/* SPDX-License-Identifier: MIT */
#include "forest_bridge_floor.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>

int bfm_forest_bridge_floor_load_at(const BfmForestField *field, uint32_t address,
                                    BfmForestBridgeFloor *floor)
{
    const uint8_t *shape = bfm_forest_room_bytes(field, address, 16);
    if (!floor || !shape || u32le(shape)) return 0;

    BfmForestBridgeFloor loaded;
    for (unsigned i = 0; i < 3; ++i) {
        loaded.box.low[i] = s16le(shape + 4 + 4 * i);
        loaded.box.high[i] = s16le(shape + 6 + 4 * i);
        if (loaded.box.low[i] >= loaded.box.high[i]) return 0;
    }
    *floor = loaded;
    return 1;
}

static int32_t q12(int64_t value)
{
    return (int32_t)(value / 4096 - (value % 4096 < 0));
}

static int32_t model_coordinate(const BfmForestTransform *root, const int16_t at[3], unsigned axis)
{
    int64_t result = 0;
    for (unsigned i = 0; i < 3; ++i)
        result += ((int64_t)at[i] - root->translation[i]) * root->rotation[i][axis];
    return q12(result);
}

int bfm_forest_bridge_floor_load(const BfmForestField *field, BfmForestBridgeFloor *floor)
{
    return bfm_forest_bridge_floor_load_at(field, UINT32_C(0x8018a8b4), floor);
}

int bfm_forest_bridge_floor_contact(const BfmForestBridgeFloor *floor,
    const BfmForestTransform *root, unsigned state, uint16_t flags, BfmForestBridgeProbe probe,
    const int16_t from[3], const int16_t to[3], BfmForestBridgeSurface *surface)
{
    if (!floor || !root || !from || !to || !surface || (state != 2 && state != 3) ||
        !(flags & 0x0400u) || root->rotation[1][2] >= -3019)
        return 0;

    int16_t start[3] = {from[0], from[1], from[2]};
    int16_t end[3] = {to[0], to[1], to[2]};
    if (probe == BFM_BRIDGE_FLOOR_STRIDE) {
        /* Attached feet sweep from three units above their previous
         * position to eight units below the proposed position. */
        if (start[1] < INT16_MIN + 3 || end[1] > INT16_MAX - 8) return 0;
        start[1] -= 3;
        end[1] += 8;
    } else if (probe != BFM_BRIDGE_FLOOR_SEGMENT) {
        return 0;
    }
    if (start[1] >= end[1]) return 0;

    /* The top of the pitched trunk is its local +Z face. A probe starting
     * inside can return a radial body contact which cannot support a foot. */
    if (model_coordinate(root, start, 2) <= floor->box.high[2]) return 0;

    int16_t center[3], contact[3], normal[3];
    if (!bfm_forest_guard_contact_point(&floor->box, root, start, end, center, contact, normal) ||
        normal[1] >= -3019) return 0;
    if (probe == BFM_BRIDGE_FLOOR_STRIDE) {
        /* On the top face, the two tangent coordinates follow the
         * proposed endpoint in the model's frame. */
        int64_t vertical = (floor->box.high[2] + 2) * (int64_t)root->rotation[1][2];
        for (unsigned i = 0; i < 2; ++i)
            vertical += (int64_t)model_coordinate(root, end, i) * root->rotation[1][i];
        int64_t height = (int64_t)root->translation[1] + q12(vertical);
        if (height < INT16_MIN || height > INT16_MAX) return 0;
        contact[1] = (int16_t)height;
    }
    *surface = (BfmForestBridgeSurface){contact[1], {normal[0], normal[1], normal[2]}};
    return 1;
}
