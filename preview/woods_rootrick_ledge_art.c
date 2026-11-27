/* SPDX-License-Identifier: MIT */
#include "woods_rootrick_ledge_art.h"
#include "woods_rootrick_wall.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>

struct BfmWoodsRootrickLedgeArt {
    const BfmRootrickWall *wall;
    BfmForestPose pose;
    uint32_t table[2];
    MusashiForestPlayerMesh *mesh[2];
};

BfmWoodsRootrickLedgeArt *bfm_woods_rootrick_ledge_art_open(const uint8_t *scene, size_t bytes,
                                                            const BfmRootrickWall *wall)
{
    const uint32_t base = UINT32_C(0x80128158);
    const size_t clip = UINT32_C(0x801958a8) - base;
    const BfmRootrickObstacleField *props = bfm_rootrick_wall_props(wall);
    if (!scene || !props || props->count != BFM_ROOTRICK_PLATFORMS || bytes < clip + 16 ||
        u32le(scene+clip+4) != 1 || u32le(scene+clip+8) || u32le(scene+clip+12) != 0xffff)
        return NULL;
    uint32_t frame = u32le(scene+clip);
    if (frame < base || (frame & 3) || frame-base > bytes-12) return NULL;
    BfmWoodsRootrickLedgeArt *art = calloc(1, sizeof *art);
    if (!art) return NULL;
    art->wall = wall;
    art->table[0] = bfm_rootrick_obstacle_model(props,0);
    art->table[1] = bfm_rootrick_obstacle_model(props,2);
    if (art->table[0] == art->table[1] || art->table[0] != bfm_rootrick_obstacle_model(props,1) ||
        !bfm_forest_actor_pose_decode_full(scene + (frame-base), 12, 1, &art->pose)) goto fail;
    for (unsigned i = 0; i < 2; ++i) {
        unsigned parts;
        art->mesh[i] = musashi_forest_player_mesh_from_table(scene, bytes, base, art->table[i], &parts);
        if (!art->mesh[i] || parts != 1) goto fail;
    }
    return art;
fail:
    bfm_woods_rootrick_ledge_art_free(art);
    return NULL;
}

void bfm_woods_rootrick_ledge_art_free(BfmWoodsRootrickLedgeArt *art)
{
    if (!art) return;
    for (unsigned i = 0; i < 2; ++i) musashi_forest_player_mesh_free(art->mesh[i]);
    free(art);
}

int bfm_woods_rootrick_ledge_art_add(const BfmWoodsRootrickLedgeArt *art, BfmBedroomFigure *figure,
                                    const BfmBedroomView *view)
{
    if (!art || !figure || !view) return 0;
    const BfmRootrickObstacleField *props = bfm_rootrick_wall_props(art->wall);
    for (unsigned slot = 0; slot < BFM_ROOTRICK_PLATFORMS; ++slot) {
        BfmForestGuardBox shape;
        BfmForestTransform root;
        uint32_t table = bfm_rootrick_obstacle_model(props,slot);
        unsigned index = table == art->table[1];
        if (table != art->table[index] || !bfm_rootrick_obstacle_body(props,slot,&shape,&root)) return 0;
        BfmForestActor actor = {.mesh=art->mesh[index], .root=&root, .pose=&art->pose, .visible_parts=1};
        if (!bfm_bedroom_figure_add_far_ft4_actor(figure,view,&actor)) return 0;
    }
    return 1;
}
