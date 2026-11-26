/* SPDX-License-Identifier: MIT */
#include "forest_shore_pose.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"

enum { SHORE_PARTS = 24, SHORE_KEYS = 20 };

static int full_pose(const uint8_t *frame, BfmForestPose *out)
{
    /* Full records store signed XYZ and three Euler angles. Express the
     * angles in the forest renderer's compact form to use precisely the
     * same fixed-point rotation; retain the full signed translations. */
    uint8_t angles[8 * SHORE_PARTS] = {0};
    for (unsigned i = 0; i < SHORE_PARTS; ++i) {
        const uint8_t *joint = frame + 12 * i;
        uint8_t *packed = angles + 8 * i;
        uint16_t y = u16le(joint + 8), z = u16le(joint + 10);
        packed[0] = (uint8_t)(((z >> 4) & 0xf0) | ((y >> 8) & 15));
        packed[1] = (uint8_t)y;
        packed[2] = (uint8_t)z;
        packed[6] = joint[6];
        packed[7] = joint[7];
    }
    if (!bfm_forest_actor_pose_decode(angles, sizeof angles, SHORE_PARTS, out))
        return 0;
    for (unsigned i = 0; i < SHORE_PARTS; ++i)
        for (unsigned axis = 0; axis < 3; ++axis)
            out->local[i].translation[axis] = s16le(frame + 12 * i + 2 * axis);
    return 1;
}

int bfm_forest_shore_pose(const BfmForestField *field, unsigned state,
                          unsigned state_tick, BfmForestPose *out)
{
    /* Each state selects a distinct mounted clip of the same 24-part
     * model. The pose storage follows its part list and ends at the first
     * clip table. Each key carries its own address and duration. */
    static const uint32_t clips[] = {0x8019731c, 0x801973c4, 0x80196f3c};
    const uint32_t pose_end = clips[2];
    const uint32_t compact_bit = UINT32_C(0x01000000);
    if (!field || !out || state < 1 || state > 3 || (state == 2 && state_tick >= SHORE_KEYS))
        return 0;
    const uint8_t *clip = bfm_forest_room_bytes(field, clips[state - 1], 8 * (SHORE_KEYS + 1));
    if (!clip || u32le(clip + 8 * SHORE_KEYS) || u32le(clip + 8 * SHORE_KEYS + 4) != 0xffff)
        return 0;
    const uint8_t *key = clip + 8 * (state_tick % SHORE_KEYS);
    uint32_t encoded = u32le(key), address = encoded & ~compact_bit;
    size_t length = SHORE_PARTS * (encoded & compact_bit ? 8u : 12u);
    if (u32le(key + 4) != 1 || address < 0x8018c9dc || address > pose_end - length)
        return 0;
    const uint8_t *frame = bfm_forest_room_bytes(field, address, length);
    if (!frame) return 0;
    if (encoded & compact_bit)
        return bfm_forest_actor_pose_decode(frame, length, SHORE_PARTS, out);
    return full_pose(frame, out);
}
