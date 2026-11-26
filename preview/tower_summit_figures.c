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
    int16_t plate[3], sword[3];
    unsigned sword_scale, extra_scale;
    unsigned visible;
    unsigned retired_ticks;
    unsigned strength, debounce;
    int last_occupied, shielded;
};

enum { STONE_HEAD, STONE_BASE, PRESSURE_PLATE, UPRIGHT_SWORD, CRYSTAL };

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t count)
{
    if (!scene || address < scene_origin) return NULL;
    size_t offset = address - scene_origin;
    return offset <= bytes && count <= bytes - offset ? scene + offset : NULL;
}

BfmTowerSummitFigures *bfm_tower_summit_figures_open(const uint8_t *scene, size_t bytes)
{
    /* The head faces west at entry. The pedestal's idle pose raises the
     * base of the head. Both pickup figures share the final placement. */
    static const struct { unsigned row, kind; uint32_t shape, clip; } entry[] = {
        {1, 41, UINT32_C(0x801af280), UINT32_C(0x8018eddc)},
        {2, 42, UINT32_C(0x801afac8), UINT32_C(0x8018f5a4)},
        {3, 45, UINT32_C(0x801dca30), 0},
        {4, 46, UINT32_C(0x801dd528), 0},
        {4, 46, UINT32_C(0x801dced0), 0}
    };
    const uint8_t *list = mounted(scene, bytes, UINT32_C(0x801a7be4), 4);
    const uint8_t *records = list ? mounted(scene, bytes, u32le(list), 5 * 20) : NULL;
    const uint8_t *scale = mounted(scene, bytes, UINT32_C(0x8018f314), 8);
    if (!records || !scale || !u32le(scale) || u32le(scale) > 8192 || u32le(scale+4) > 8192) return NULL;
    BfmTowerSummitFigures *figures = calloc(1, sizeof *figures);
    if (!figures) return NULL;
    figures->sword_scale = u32le(scale);
    figures->extra_scale = u32le(scale + 4);
    for (unsigned i = 0; i < BFM_TOWER_SUMMIT_FIGURES; ++i) {
        const uint8_t *row = records + entry[i].row * 20;
        if (u16le(row + 6) != entry[i].kind || u16le(row + 8)) goto failed;
        SummitFigure *figure = &figures->figure[i];
        unsigned parts = 0;
        figure->mesh = musashi_forest_player_mesh_from_table(scene, bytes, scene_origin,
                                                              entry[i].shape, &parts);
        if (!figure->mesh || parts != 1) goto failed;
        int16_t position[3], local[3] = {0};
        for (unsigned axis = 0; axis < 3; ++axis) position[axis] = s16le(row + 2*axis);
        bfm_forest_actor_root(i == STONE_HEAD ? 3072 : u16le(row + 12), position, &figure->root);
        if (i == PRESSURE_PLATE || i == UPRIGHT_SWORD) {
            int16_t *target = i == PRESSURE_PLATE ? figures->plate : figures->sword;
            for (unsigned axis = 0; axis < 3; ++axis) target[axis] = position[axis];
        }
        if (entry[i].clip) {
            const uint8_t *clip = mounted(scene, bytes, entry[i].clip, 8);
            const uint8_t *frame = clip ? mounted(scene, bytes, u32le(clip), 12) : NULL;
            if (!frame || u32le(clip + 4) != 1 || u16le(frame + 6) || u32le(frame + 8))
                goto failed;
            for (unsigned axis = 0; axis < 3; ++axis) local[axis] = s16le(frame + 2*axis);
        }
        bfm_forest_actor_root(0, local, &figure->pose.local[0]);
    }
    figures->strength = 16;
    figures->shielded = 1;
    bfm_tower_summit_figures_present_sword(figures, 0, figures->strength, figures->shielded, 1);
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

int bfm_tower_summit_figures_targets(const BfmTowerSummitFigures *figures, int16_t plate[3], int16_t sword[3])
{
    if (!figures || !plate || !sword) return 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        plate[axis] = figures->plate[axis];
        sword[axis] = figures->sword[axis];
    }
    return 1;
}

void bfm_tower_summit_figures_present_sword(BfmTowerSummitFigures *figures, unsigned tick,
                                           unsigned settle, int shielded, int sword_present)
{
    if (!figures || settle > 16) return;
    if (!sword_present) {
        /* The last submitted upright blade is still visible in two
         * successive captured updates after the pickup actor disappears. */
        if (figures->retired_ticks < 3) ++figures->retired_ticks;
        if (figures->retired_ticks == 3) figures->visible = (1u << UPRIGHT_SWORD) - 1;
        return;
    }
    figures->retired_ticks = 0;
    figures->visible = ((1u << CRYSTAL) - 1) | ((unsigned)!!shielded << CRYSTAL);
    bfm_forest_actor_root((tick * 16u) & 4095u, figures->sword, &figures->figure[CRYSTAL].root);
    BfmForestTransform *blade = &figures->figure[UPRIGHT_SWORD].pose.local[0];
    int16_t scale = (int16_t)(figures->sword_scale + figures->extra_scale * settle / 16);
    for (unsigned axis = 0; axis < 3; ++axis) blade->rotation[axis][axis] = scale;
}

void bfm_tower_summit_figures_update_sword(BfmTowerSummitFigures *figures, unsigned tick,
                                          int plate_occupied, int sword_present)
{
    if (!figures) return;
    if (sword_present) {
        plate_occupied = !!plate_occupied;
        if (plate_occupied != figures->last_occupied) {
            figures->last_occupied = plate_occupied;
            figures->debounce = 8;
        }
        if (figures->debounce) --figures->debounce;
        else figures->shielded = !plate_occupied;
    }
    /* Display the scale accumulated on the previous update. A stable plate
     * can drop the shield immediately while the sword is still shrinking. */
    bfm_tower_summit_figures_present_sword(figures, tick, figures->strength,
                                          figures->shielded, sword_present);
    if (sword_present) {
        if (figures->shielded && figures->strength < 16) ++figures->strength;
        if (!figures->shielded && figures->strength) --figures->strength;
    }
}

size_t bfm_tower_summit_figures_actors(const BfmTowerSummitFigures *figures,
                                     BfmForestActor *out, size_t capacity)
{
    if (!figures || !out || capacity < BFM_TOWER_SUMMIT_FIGURES) return 0;
    for (unsigned i = 0; i < BFM_TOWER_SUMMIT_FIGURES; ++i) {
        const SummitFigure *figure = &figures->figure[i];
        out[i] = (BfmForestActor){ .mesh = figure->mesh, .root = &figure->root,
                                   .pose = &figure->pose, .visible_parts = (figures->visible >> i) & 1u };
    }
    return BFM_TOWER_SUMMIT_FIGURES;
}
