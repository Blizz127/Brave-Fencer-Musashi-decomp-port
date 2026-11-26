/* SPDX-License-Identifier: MIT */
#include "tower_summit_figures.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>

static const uint32_t scene_origin = UINT32_C(0x80128158);

typedef struct {
    MusashiForestPlayerMesh *mesh;
    BfmForestTransform root;
    BfmForestPose pose;
} SummitFigure;

struct BfmTowerSummitFigures {
    SummitFigure figure[BFM_TOWER_SUMMIT_FIGURES];
};

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t count)
{
    if (!scene || address < scene_origin) return NULL;
    size_t offset = address - scene_origin;
    return offset <= bytes && count <= bytes - offset ? scene + offset : NULL;
}

BfmTowerSummitFigures *bfm_tower_summit_figures_open(const uint8_t *scene, size_t bytes)
{
    /* These three adjacent placements each bind a one-part model. The head
     * faces west on the first visit. The pedestal's idle pose lifts its local
     * origin from the placement to the base of the head. */
    static const struct { unsigned kind; uint32_t shape, clip; } entry[] = {
        {41, UINT32_C(0x801af280), UINT32_C(0x8018eddc)},
        {42, UINT32_C(0x801afac8), UINT32_C(0x8018f5a4)},
        {45, UINT32_C(0x801dca30), 0}
    };
    const uint8_t *list = mounted(scene, bytes, UINT32_C(0x801a7be4), 4);
    const uint8_t *records = list ? mounted(scene, bytes, u32le(list), 4 * 20) : NULL;
    if (!records) return NULL;
    BfmTowerSummitFigures *figures = calloc(1, sizeof *figures);
    if (!figures) return NULL;
    for (unsigned i = 0; i < BFM_TOWER_SUMMIT_FIGURES; ++i) {
        const uint8_t *row = records + (i + 1) * 20;
        if (u16le(row + 6) != entry[i].kind || u16le(row + 8)) goto failed;
        SummitFigure *figure = &figures->figure[i];
        unsigned parts = 0;
        figure->mesh = musashi_forest_player_mesh_from_table(scene, bytes, scene_origin,
                                                              entry[i].shape, &parts);
        if (!figure->mesh || parts != 1) goto failed;
        int16_t position[3], local[3] = {0};
        for (unsigned axis = 0; axis < 3; ++axis) position[axis] = s16le(row + 2*axis);
        bfm_forest_actor_root(i == 0 ? 3072 : u16le(row + 12), position, &figure->root);
        if (entry[i].clip) {
            const uint8_t *clip = mounted(scene, bytes, entry[i].clip, 8);
            const uint8_t *frame = clip ? mounted(scene, bytes, u32le(clip), 12) : NULL;
            if (!frame || u32le(clip + 4) != 1 || u16le(frame + 6) || u32le(frame + 8))
                goto failed;
            for (unsigned axis = 0; axis < 3; ++axis) local[axis] = s16le(frame + 2*axis);
        }
        bfm_forest_actor_root(0, local, &figure->pose.local[0]);
    }
    return figures;
failed:
    bfm_tower_summit_figures_free(figures);
    return NULL;
}

void bfm_tower_summit_figures_free(BfmTowerSummitFigures *figures)
{
    if (!figures) return;
    for (unsigned i = 0; i < BFM_TOWER_SUMMIT_FIGURES; ++i)
        musashi_forest_player_mesh_free(figures->figure[i].mesh);
    free(figures);
}

size_t bfm_tower_summit_figures_actors(const BfmTowerSummitFigures *figures,
                                     BfmForestActor *out, size_t capacity)
{
    if (!figures || !out || capacity < BFM_TOWER_SUMMIT_FIGURES) return 0;
    for (unsigned i = 0; i < BFM_TOWER_SUMMIT_FIGURES; ++i) {
        const SummitFigure *figure = &figures->figure[i];
        out[i] = (BfmForestActor){ .mesh = figure->mesh, .root = &figure->root,
                                   .pose = &figure->pose, .visible_parts = 1 };
    }
    return BFM_TOWER_SUMMIT_FIGURES;
}
