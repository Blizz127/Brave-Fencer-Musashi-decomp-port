/* SPDX-License-Identifier: MIT */
#include "forest_glow_figures.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>

enum { WINGS = 4 };

struct BfmForestGlowFigures {
    MusashiForestPlayerMesh *mesh;
    BfmForestPose pose[BFM_FOREST_GLOW_KEYS];
};

static int load_keys(BfmForestGlowFigures *figures, const BfmForestField *field)
{
    const uint8_t *clip = bfm_forest_room_bytes(field, UINT32_C(0x8018a800), 8 * (BFM_FOREST_GLOW_KEYS + 1));
    if (!clip || u32le(clip + 8 * BFM_FOREST_GLOW_KEYS) || u32le(clip + 8 * BFM_FOREST_GLOW_KEYS + 4) != 0xffff) return 0;
    for (unsigned i = 0; i < BFM_FOREST_GLOW_KEYS; ++i) {
        const uint8_t *key = bfm_forest_room_bytes(field, u32le(clip + 8 * i), 12 * WINGS);
        if (!key || u32le(clip + 8 * i + 4) != 1) return 0;
        for (unsigned j = 0; j < WINGS; ++j) {
            const uint8_t *joint = key + j * 12;
            if (s16le(joint + 6) || s16le(joint + 8)) return 0;
            int roll = s16le(joint + 10);
            int16_t position[3] = {s16le(joint), s16le(joint + 2), s16le(joint + 4)};
            BfmForestTransform angle;
            bfm_forest_actor_root((unsigned)roll, position, &angle);
            int c = angle.rotation[0][0], s = angle.rotation[0][2];
            figures->pose[i].local[j] = (BfmForestTransform){
                .rotation = {{c, -s, 0}, {s, c, 0}, {0, 0, 4096}},
                .translation = {position[0], position[1], position[2]}
            };
        }
    }
    return 1;
}

BfmForestGlowFigures *bfm_forest_glow_figures_open(const BfmForestField *field)
{
    const uint32_t table = UINT32_C(0x801d20b4);
    const uint8_t *parts = bfm_forest_room_bytes(field, table, 4 * (WINGS + 1));
    if (!parts || u32le(parts + 4 * WINGS)) return NULL;
    uint32_t first = u32le(parts);
    if (first >= table) return NULL;
    size_t size = table - first + 4 * (WINGS + 1);
    const uint8_t *block = bfm_forest_room_bytes(field, first, size);
    if (!block) return NULL;
    BfmForestGlowFigures *figures = calloc(1, sizeof(*figures));
    if (!figures) return NULL;
    unsigned count = 0;
    figures->mesh = musashi_forest_player_mesh_from_table(block, size, first, table, &count);
    if (count != WINGS || !figures->mesh || !load_keys(figures, field)) {
        bfm_forest_glow_figures_free(figures);
        return NULL;
    }
    return figures;
}

void bfm_forest_glow_figures_free(BfmForestGlowFigures *figures)
{
    if (figures) musashi_forest_player_mesh_free(figures->mesh);
    free(figures);
}

unsigned bfm_forest_glow_figures_collect(const BfmForestGlowFigures *figures,
                                         const BfmForestGlows *lights,
                                         BfmForestActor out[BFM_FOREST_GLOWS])
{
    if (!figures || !lights || !out) return 0;
    for (unsigned i = 0; i < lights->count; ++i) {
        const BfmForestGlow *light = &lights->glows[lights->order[i]];
        out[i] = (BfmForestActor){.mesh = figures->mesh, .pose = &figures->pose[light->key],
            .root = &light->figure, .visible_parts = (1u << WINGS) - 1};
    }
    return lights->count;
}
