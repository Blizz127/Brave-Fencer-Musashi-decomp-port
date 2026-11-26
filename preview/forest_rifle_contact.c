/* SPDX-License-Identifier: MIT */
#include "forest_rifle_contact.h"
#include "forest_guard_contact.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"

/* Each attack tests a segment on the attacker's completed model transform
 * against the player's world-aligned default body. */
static int hits_player(const BfmForestField *field, const BfmForestTransform *attacker,
                       const BfmForestTransform *player, const int16_t local[2][3])
{
    const uint8_t *shape = bfm_forest_room_bytes(field, UINT32_C(0x801884cc), 16);
    if (!shape || u32le(shape)) return 0;
    BfmForestGuardBox box;
    BfmForestTransform body = {0};
    int16_t segment[2][3], center[3];
    for (unsigned axis = 0; axis < 3; ++axis) {
        box.low[axis] = s16le(shape + 4 + axis * 4);
        box.high[axis] = s16le(shape + 6 + axis * 4);
        if (box.low[axis] > box.high[axis]) return 0;
        body.rotation[axis][axis] = 4096;
        body.translation[axis] = player->translation[axis];
        for (unsigned end = 0; end < 2; ++end) {
            int64_t dot = 0;
            for (unsigned k = 0; k < 3; ++k)
                dot += (int64_t)attacker->rotation[axis][k] * local[end][k];
            int64_t position = attacker->translation[axis] + dot / 4096 - (dot % 4096 < 0);
            if (position < INT16_MIN || position > INT16_MAX) return 0;
            segment[end][axis] = (int16_t)position;
        }
    }
    return bfm_forest_guard_contact(&box, &body, segment[0], segment[1], center);
}

int bfm_forest_rifle_hits_player(const BfmForestField *field,
                                 const BfmForestTransform *projectile,
                                 const BfmForestTransform *player,
                                 int contact_suppressed)
{
    if (!field || !projectile || !player || contact_suppressed) return 0;
    const uint8_t *offset = bfm_forest_room_bytes(field, UINT32_C(0x801b129c), 8);
    if (!offset) return 0;
    int16_t local[2][3] = {0};
    for (unsigned i = 0; i < 3; ++i) local[0][i] = s16le(offset + 2 * i);
    return hits_player(field, projectile, player, local);
}

static int mounted_segment_hits(const BfmForestField *field, uint32_t descriptor,
                                const BfmForestTransform *attacker,
                                const BfmForestTransform *player)
{
    if (!field || !attacker || !player) return 0;
    const uint8_t *points = bfm_forest_room_bytes(field, descriptor, 16);
    if (!points) return 0;
    int16_t local[2][3];
    for (unsigned i = 0; i < 2; ++i)
        for (unsigned axis = 0; axis < 3; ++axis)
            local[i][axis] = s16le(points + 8 * i + 2 * axis);
    return hits_player(field, attacker, player, local);
}

int bfm_forest_soldier_hits_player(const BfmForestField *field,
                                   const BfmForestTransform *soldier,
                                   const BfmForestTransform *player)
{
    return mounted_segment_hits(field, UINT32_C(0x8018aad4), soldier, player);
}

int bfm_forest_mouth_hits_player(const BfmForestField *field,
                                 const BfmForestTransform *figure,
                                 const BfmForestTransform *player)
{
    return mounted_segment_hits(field, UINT32_C(0x8018af78), figure, player);
}
