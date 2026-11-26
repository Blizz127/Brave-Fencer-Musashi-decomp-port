/* SPDX-License-Identifier: MIT */
#include "forest_shore_pose.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"

enum { SHORE_PARTS = 24 };

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

/* The mounted model has eight lists between its pose storage and the next
 * mesh. Each key's original encoding selects full or compact transforms. */
static const struct { uint32_t start; unsigned keys; } shore_clips[] = {
    {0x80196f3c, 20}, {0x80196fe4, 30}, {0x801970dc, 50}, {0x80197274, 20},
    {0x8019731c, 20}, {0x801973c4, 20}, {0x8019746c, 20}, {0x80197514, 15}
};

int bfm_forest_shore_clip_pose(const BfmForestField *field, uint32_t clip_address,
                               unsigned index, BfmForestPose *out)
{
    unsigned keys = 0;
    for (size_t i = 0; i < sizeof shore_clips / sizeof shore_clips[0]; ++i)
        if (shore_clips[i].start == clip_address) keys = shore_clips[i].keys;
    if (!out || index >= keys) return 0;
    const uint8_t *clip = bfm_forest_room_bytes(field, clip_address, 8 * (keys + 1));
    if (!clip || u32le(clip + 8 * keys) || u32le(clip + 8 * keys + 4) != 0xffff)
        return 0;
    const uint8_t *key = clip + 8 * index;
    uint32_t encoded = u32le(key), compact = UINT32_C(0x01000000);
    uint32_t address = encoded & ~compact;
    size_t length = SHORE_PARTS * (encoded & compact ? 8u : 12u);
    if (u32le(key + 4) != 1 || address < 0x8018c9dc || address > shore_clips[0].start - length)
        return 0;
    const uint8_t *frame = bfm_forest_room_bytes(field, address, length);
    if (!frame) return 0;
    if (encoded & compact)
        return bfm_forest_actor_pose_decode(frame, length, SHORE_PARTS, out);
    return full_pose(frame, out);
}

int bfm_forest_shore_pose(const BfmForestField *field, unsigned state,
                          unsigned state_tick, BfmForestPose *out)
{
    static const uint32_t clips[] = {0x8019731c, 0x801973c4, 0x80196f3c};
    if (state < 1 || state > 3 || (state == 2 && state_tick >= 20)) return 0;
    return bfm_forest_shore_clip_pose(field, clips[state - 1], state_tick % 20, out);
}
