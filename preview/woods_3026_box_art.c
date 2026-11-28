/* SPDX-License-Identifier: MIT */
#include "woods_3026_box_art.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

struct BfmWoods3026BoxArt {
    MusashiForestPlayerMesh *mesh[2];
    BfmForestTransform root[2];
    uint8_t lid_key[BFM_WOODS_3026_BOX_OPEN + 1][8];
};

static const uint32_t origin = UINT32_C(0x80128158);

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t size)
{
    if (!scene || address < origin || address - origin > bytes || size > bytes - (address - origin))
        return NULL;
    return scene + (address - origin);
}

static const uint8_t *box_placement(const uint8_t *scene, size_t bytes, uint32_t room_address)
{
    const uint8_t *room = mounted(scene, bytes, room_address, 0xbc);
    if (!room) return NULL;
    uint32_t first = u32le(room + 0xb4), end = u32le(room + 0xb8);
    if (end <= first || (end - first) % 20) return NULL;
    const uint8_t *rows = mounted(scene, bytes, first, end - first);
    if (!rows) return NULL;
    for (size_t i = 0; i < end - first; i += 20) {
        const uint8_t *row = rows + i;
        unsigned kind = u16le(row + 6);
        if (!kind) break;
        if (kind == 483 && !u16le(row + 8)) return row;
    }
    return NULL;
}

static int read_lid_keys(BfmWoods3026BoxArt *art, const uint8_t *scene, size_t bytes, uint32_t keys)
{
    const unsigned ticks = BFM_WOODS_3026_BOX_OPEN;
    const uint8_t *idle = mounted(scene, bytes, keys, 16);
    const uint8_t *open = mounted(scene, bytes, keys + 16, (ticks + 1) * 8);
    const uint8_t *close = mounted(scene, bytes, keys + 16 + (ticks + 1) * 8, (ticks + 1) * 8);
    if (!idle || !open || !close || u32le(idle + 8) || u32le(idle + 12) != 0xffff ||
        u32le(open + ticks * 8) || u32le(open + ticks * 8 + 4) != 0xfffe ||
        u32le(close + ticks * 8) || u32le(close + ticks * 8 + 4) != 0xfffe) return 0;
    for (unsigned i = 0; i <= ticks; ++i) {
        const uint8_t *step = i ? open + 8 * (i - 1) : idle;
        uint32_t key = u32le(step);
        if (u32le(step + 4) != 1 || (key >> 24) != 0x81) return 0;
        if (i && memcmp(step, close + 8 * (ticks - i), 8)) return 0;
        const uint8_t *pose = mounted(scene, bytes, key & ~UINT32_C(0x01000000), 8);
        if (!pose) return 0;
        memcpy(art->lid_key[i], pose, 8);
    }
    return 1;
}

static BfmWoods3026BoxArt *load_art(const uint8_t *scene, size_t bytes, uint32_t room,
                                    uint32_t profiles_at, uint32_t keys)
{
    const uint8_t *row = box_placement(scene, bytes, room);
    const uint8_t *profiles = mounted(scene, bytes, profiles_at, 2 * 52);
    if (!row || !profiles) return NULL;
    BfmWoods3026BoxArt *art = calloc(1, sizeof *art);
    if (!art) return NULL;
    if (!read_lid_keys(art, scene, bytes, keys)) goto fail;
    int16_t xyz[3] = {s16le(row), s16le(row + 2), s16le(row + 4)};
    for (unsigned i = 0; i < 2; ++i) {
        unsigned count;
        uint32_t table = u32le(profiles + 52 * i + 4);
        art->mesh[i] = musashi_forest_player_mesh_from_table(scene, bytes, origin, table, &count);
        if (!art->mesh[i] || count != 1) goto fail;
        /* The child shares the base position and faces back across the hinge. */
        bfm_forest_actor_root((u16le(row + 12) + i * 2048u) & 4095u, xyz, &art->root[i]);
    }
    return art;
fail:
    bfm_woods_3026_box_art_free(art);
    return NULL;
}

BfmWoods3026BoxArt *bfm_woods_3026_box_art_load(const uint8_t *scene, size_t bytes)
{
    return load_art(scene, bytes, UINT32_C(0x801961e8), UINT32_C(0x801e8554), UINT32_C(0x801e38a0));
}

BfmWoods3026BoxArt *bfm_woods_3026_box_art_load_shaft(const uint8_t *scene, size_t bytes)
{
    return load_art(scene, bytes, UINT32_C(0x80181f28), UINT32_C(0x80197174), UINT32_C(0x80197734));
}

void bfm_woods_3026_box_art_free(BfmWoods3026BoxArt *art)
{
    if (!art) return;
    musashi_forest_player_mesh_free(art->mesh[0]);
    musashi_forest_player_mesh_free(art->mesh[1]);
    free(art);
}

int bfm_woods_3026_box_art_draw(const BfmWoods3026BoxArt *art, BfmBedroomFigure *figure,
                               const BfmBedroomView *view, unsigned openness)
{
    static const BfmForestPose body = {.local = {{.rotation = {{4096,0,0},{0,4096,0},{0,0,4096}}}}};
    BfmForestPose lid;
    if (!art || openness > BFM_WOODS_3026_BOX_OPEN ||
        !bfm_forest_actor_pose_decode(art->lid_key[openness], 8, 1, &lid)) return 0;
    const BfmForestActor base = {
        .mesh = art->mesh[0], .root = &art->root[0], .pose = &body, .visible_parts = 1
    };
    const BfmForestActor cover = {
        .mesh = art->mesh[1], .root = &art->root[1], .pose = &lid, .visible_parts = 1
    };
    return bfm_bedroom_figure_add_actor(figure, view, &base, NULL, NULL, 0) &&
           bfm_bedroom_figure_add_actor(figure, view, &cover, NULL, NULL, 0);
}
