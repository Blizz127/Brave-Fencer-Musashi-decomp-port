/* SPDX-License-Identifier: MIT */
#include "woods_lily_art.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>

struct BfmWoodsLilyArt {
    MusashiForestPlayerMesh *mesh;
    BfmForestPose pose;
};

static const uint32_t scene_origin = UINT32_C(0x80128158);

static const uint8_t *mounted(const uint8_t *scene, size_t bytes,
                              uint32_t address, size_t length)
{
    if (!scene || address < scene_origin) return NULL;
    size_t offset = address - scene_origin;
    return offset <= bytes && length <= bytes - offset ? scene + offset : NULL;
}

BfmWoodsLilyArt *bfm_woods_lily_art_load(const uint8_t *scene, size_t bytes)
{
    /* SC03.CD member 29, class 0x94. The resource profile names the mesh
     * table. Its one-key loop uses a full XYZ translation and zero joint
     * angles. Both are relative to the platform's moving root. */
    const uint8_t *profile = mounted(scene, bytes, UINT32_C(0x801c4200), 8);
    const uint8_t *clip = mounted(scene, bytes, UINT32_C(0x801c4240), 16);
    if (!profile || !clip || u32le(clip + 4) != 1 || u32le(clip + 8) ||
        u32le(clip + 12) != 0xffff) return NULL;
    const uint8_t *joint = mounted(scene, bytes, u32le(clip), 12);
    if (!joint || u16le(joint + 6) || u16le(joint + 8) || u16le(joint + 10)) return NULL;
    BfmWoodsLilyArt *art = calloc(1, sizeof *art);
    if (!art) return NULL;
    unsigned parts;
    art->mesh = musashi_forest_player_mesh_from_table(scene, bytes, scene_origin,
                                                       u32le(profile + 4), &parts);
    if (!art->mesh || parts != 1) {
        bfm_woods_lily_art_free(art);
        return NULL;
    }
    int16_t offset[3] = {s16le(joint), s16le(joint + 2), s16le(joint + 4)};
    bfm_forest_actor_root(0, offset, &art->pose.local[0]);
    return art;
}

void bfm_woods_lily_art_free(BfmWoodsLilyArt *art)
{
    if (!art) return;
    musashi_forest_player_mesh_free(art->mesh);
    free(art);
}

int bfm_woods_lily_art_append(const BfmWoodsLilyArt *art, BfmBedroomFigure *figure,
                             const BfmBedroomView *view, const BfmForestTransform *root)
{
    if (!art || !root) return 0;
    BfmForestActor pad = {.mesh = art->mesh, .pose = &art->pose, .root = root, .visible_parts = 1};
    return bfm_bedroom_figure_add_fusion(figure, view, &pad, NULL, 0);
}
