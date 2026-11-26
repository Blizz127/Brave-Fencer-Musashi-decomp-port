/* SPDX-License-Identifier: MIT */
#include "steam_knight_cast.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>

static const uint32_t scene_origin = UINT32_C(0x80128158);

static int decode_pose(const uint8_t *frame, unsigned parts, BfmForestPose *pose)
{
    /* Full joint records contain signed XYZ followed by X/Y/Z angles. Use
     * the field renderer's existing fixed-point Euler conversion. */
    uint8_t angles[8 * MUSASHI_FOREST_PLAYER_PARTS] = {0};
    for (unsigned i = 0; i < parts; ++i) {
        const uint8_t *joint = frame + 12 * i;
        uint8_t *packed = angles + 8 * i;
        uint16_t y = u16le(joint + 8), z = u16le(joint + 10);
        packed[0] = (uint8_t)(((z >> 4) & 0xf0) | ((y >> 8) & 15));
        packed[1] = (uint8_t)y;
        packed[2] = (uint8_t)z;
        packed[6] = joint[6];
        packed[7] = joint[7];
    }
    if (!bfm_forest_actor_pose_decode(angles, sizeof angles, parts, pose))
        return 0;
    for (unsigned i = 0; i < parts; ++i)
        for (unsigned axis = 0; axis < 3; ++axis)
            pose->local[i].translation[axis] = s16le(frame + 12 * i + 2 * axis);
    return 1;
}

enum { CAST_MEMBER = 9, CAST_SECTOR = 2048, CAST_PAYLOAD = 70784 };
static const uint32_t cast_origin = UINT32_C(0x801e4c60);

struct BfmSteamKnightHostages {
    uint8_t payload[CAST_PAYLOAD];
    MusashiForestPlayerMesh *mesh[2];
};

static const struct {
    unsigned table, parts, first_clip, clips;
} performer[] = {
    [BFM_STEAM_ROOTRICK] = {0x52ac, 16, 0, 7},
    [BFM_STEAM_FILLET] = {0xf0dc, 8, 7, 3}
};

/* The room event member stores separate key and termination records.
 * The low word of the terminal command selects a loop or a final hold. */
static const struct { unsigned offset, count; } cast_clips[] = {
    {0x1154, 10}, {0x11ac, 30}, {0x12a4, 30}, {0x139c, 26},
    {0x1474, 50}, {0x160c, 20}, {0x16b4, 21},
    {0x1764, 16}, {0x17ec, 50}, {0x1984, 10}
};

static int read_cast_member(const MusashiDiscMedia *disc, uint8_t *out)
{
    size_t bytes;
    uint8_t directory[CAST_SECTOR], header[16];
    if (!musashi_disc_media_read_root_range(disc, "SC02.CD", 0, directory,
                                            sizeof directory, &bytes))
        return 0;
    uint32_t members = u32le(directory);
    if (members <= CAST_MEMBER || members > (CAST_SECTOR - 8) / 8) return 0;
    const uint8_t *row = directory + 8 + 8 * CAST_MEMBER;
    uint32_t sector = u32le(row), extent = u32le(row + 4);
    if (!sector || sector > bytes / CAST_SECTOR) return 0;
    size_t base = (size_t)sector * CAST_SECTOR;
    if (extent > bytes - base || extent < CAST_SECTOR + CAST_PAYLOAD ||
        !musashi_disc_media_read_root_range(disc, "SC02.CD", base, header, sizeof header, &bytes) ||
        u32le(header) != 0x434150 || u32le(header + 4) != 1 || u32le(header + 8) ||
        u32le(header + 12) != CAST_SECTOR + CAST_PAYLOAD)
        return 0;
    return musashi_disc_media_read_root_range(disc, "SC02.CD", base + CAST_SECTOR,
                                              out, CAST_PAYLOAD, &bytes);
}

BfmSteamKnightHostages *bfm_steam_knight_hostages_load(const MusashiDiscMedia *disc, size_t scene_bytes)
{
    size_t capacity = cast_origin - scene_origin;
    if (!disc || scene_bytes > capacity || capacity - scene_bytes >= 16) return NULL;
    BfmSteamKnightHostages *cast = calloc(1, sizeof *cast);
    if (!cast) return NULL;
    if (!read_cast_member(disc, cast->payload)) goto fail;
    for (unsigned who = 0; who < 2; ++who) {
        unsigned parts = 0;
        uint32_t table = cast_origin + performer[who].table;
        cast->mesh[who] = musashi_forest_player_mesh_from_table(
            cast->payload, sizeof cast->payload, cast_origin, table, &parts);
        if (!cast->mesh[who] || parts != performer[who].parts)
            goto fail;
    }
    return cast;
fail:
    bfm_steam_knight_hostages_free(cast);
    return NULL;
}

void bfm_steam_knight_hostages_free(BfmSteamKnightHostages *cast)
{
    if (!cast) return;
    for (unsigned who = 0; who < 2; ++who)
        musashi_forest_player_mesh_free(cast->mesh[who]);
    free(cast);
}

int bfm_steam_knight_hostage_actor(const BfmSteamKnightHostages *cast,
                                  enum BfmSteamKnightPerformer who, unsigned clip,
                                  unsigned elapsed, const BfmForestTransform *root,
                                  BfmForestPose *pose, BfmForestActor *out)
{
    if (!cast || (unsigned)who >= 2 || clip >= performer[who].clips || !root || !pose || !out)
        return 0;
    unsigned which = performer[who].first_clip + clip;
    unsigned count = cast_clips[which].count;
    const uint8_t *keys = cast->payload + cast_clips[which].offset;
    const uint8_t *end = keys + 8 * count;
    uint32_t stop = u32le(end + 4);
    if (u32le(end) || (stop != 0xffff && stop != 0xfffe)) return 0;
    unsigned key = stop == 0xffff ? elapsed % count : (elapsed >= count ? count - 1 : elapsed);
    uint32_t encoded = u32le(keys + 8 * key);
    uint32_t address = encoded & ~UINT32_C(0x01000000);
    int compact = !!(encoded & UINT32_C(0x01000000));
    unsigned parts = performer[who].parts, length = parts * (compact ? 8u : 12u);
    uint32_t first = cast_origin + (who == BFM_STEAM_ROOTRICK ? 0x52f0u : 0xf100u);
    uint32_t limit = cast_origin + (who == BFM_STEAM_ROOTRICK ? 0xcd30u : CAST_PAYLOAD);
    if (u32le(keys + 8 * key + 4) != 1 || (address & 3) ||
        address < first || address > limit - length)
        return 0;
    const uint8_t *frame = cast->payload + (address - cast_origin);
    if (compact ? !bfm_forest_actor_pose_decode(frame, length, parts, pose) :
                  !decode_pose(frame, parts, pose))
        return 0;
    *out = (BfmForestActor){.mesh = cast->mesh[who], .pose = pose, .root = root,
                            .visible_parts = (UINT32_C(1) << parts) - 1};
    return 1;
}
