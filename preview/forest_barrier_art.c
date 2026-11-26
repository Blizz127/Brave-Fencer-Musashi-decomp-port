/* SPDX-License-Identifier: MIT */
#include "forest_barrier_art.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>

struct BfmForestBarrierArt {
    MusashiForestPlayerMesh *mesh;
    BfmForestPose pose;
};

BfmForestBarrierArt *bfm_forest_barrier_art_open(const BfmForestField *field)
{
    const uint32_t table = UINT32_C(0x801d0b94);
    const uint8_t *parts = bfm_forest_room_bytes(field, table, 8);
    if (!parts || u32le(parts + 4)) return NULL;
    uint32_t first = u32le(parts);
    if (first >= table) return NULL;
    size_t size = (size_t)(table - first) + 8;
    const uint8_t *data = bfm_forest_room_bytes(field, first, size);
    if (!data) return NULL;

    BfmForestBarrierArt *art = calloc(1, sizeof *art);
    if (!art) return NULL;
    unsigned count = 0;
    art->mesh = musashi_forest_player_mesh_from_table(data, size, first, table, &count);
    if (!art->mesh || count != 1) {
        bfm_forest_barrier_art_free(art);
        return NULL;
    }
    for (unsigned axis = 0; axis < 3; ++axis)
        art->pose.local[0].rotation[axis][axis] = 4096;
    return art;
}

void bfm_forest_barrier_art_free(BfmForestBarrierArt *art)
{
    if (art) musashi_forest_player_mesh_free(art->mesh);
    free(art);
}

int bfm_forest_barrier_art_actor(const BfmForestBarrierArt *art,
                                const BfmForestTransform *root, BfmForestActor *out)
{
    if (!art || !root || !out) return 0;
    *out = (BfmForestActor){.mesh = art->mesh, .pose = &art->pose,
                            .root = root, .visible_parts = 1};
    return 1;
}
