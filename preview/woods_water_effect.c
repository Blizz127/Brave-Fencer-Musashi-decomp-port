/* SPDX-License-Identifier: MIT */
#include "woods_water_effect.h"
#include "woods_fusion_art.h"
#include "../pc_port/forest_player_bytes.h"

int bfm_woods_water_effect_load(BfmForestWaterEffect *effect,
                                const uint8_t *scene, size_t bytes)
{
    const size_t descriptor_slot = UINT32_C(0x801877c4) - UINT32_C(0x80128158);
    const size_t cells = UINT32_C(0x8018b1fc) - UINT32_C(0x80128158);
    if (!scene || bytes < cells + 24 ||
        u32le(scene + descriptor_slot) != UINT32_C(0x8018ea38))
        return 0;
    return bfm_forest_water_effect_load_cells(effect, scene + cells, bytes - cells);
}

int bfm_woods_water_effect_append(const BfmForestWaterEffect *effect,
                                  BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    if (!effect || !figure || !view) return 0;
    if (!effect->active) return 1;
    BfmForestCamera camera;
    if (!bfm_woods_effect_camera(view, &camera)) return 0;
    BfmForestDustCard cards[BFM_FOREST_WATER_EFFECT_CARDS];
    size_t count = bfm_forest_water_effect_project(effect, &camera, cards,
                                                  BFM_FOREST_WATER_EFFECT_CARDS);
    return !count || bfm_bedroom_figure_add_fusion(figure, view, NULL, cards, count);
}
