/* SPDX-License-Identifier: MIT */
#include "steam_knight_effects.h"
#include <string.h>

int bfm_steam_weapons_load(BfmSteamWeaponEffects *effects, const uint8_t *scene, size_t size)
{
    const size_t fusion = 0x8019381cu - 0x80128158u;
    const size_t lumina = 0x801939c8u - 0x80128158u;
    if (!effects || !scene || size < lumina + sizeof effects->lumina_rim ||
        !bfm_castle_blade_load(&effects->fusion, scene, size, fusion)) return 0;
    memcpy(effects->lumina_rim, scene + lumina, sizeof effects->lumina_rim);
    return 1;
}

int bfm_steam_weapons_tick(BfmSteamWeaponEffects *effects, const BfmSteamFlameRay *lumina)
{
    /* The displayed blade follows its logical attack sample by one update. */
    if (effects->has_edge) {
        if (!effects->trail.energy)
            effects->trail = (BfmForestSweep){.energy = 244, .divisor = 48};
        if (!bfm_forest_sweep_append(&effects->trail, &effects->next_edge)) return 0;
    } else if (effects->trail.energy && !bfm_forest_sweep_append(&effects->trail, NULL)) {
        return 0;
    }
    effects->has_edge = lumina != NULL;
    if (lumina) {
        memcpy(effects->next_edge.point[0], lumina->to, sizeof lumina->to);
        memcpy(effects->next_edge.point[1], lumina->from, sizeof lumina->from);
    }
    return 1;
}

size_t bfm_steam_weapons_project(const BfmSteamWeaponEffects *effects, const BfmForestCamera *camera,
    const BfmForestTransform *root, uint32_t visible, const int8_t fusion[2][3],
    const int8_t lumina[2][3], unsigned tick, BfmForestBladeFrame *blade, int *has_blade,
    BfmForestDustCard out[BFM_STEAM_WEAPON_CARDS])
{
    size_t count = bfm_forest_sweep_project(&effects->trail, camera, out);
    for (size_t i = 0; i < count; ++i)
        for (unsigned j = 0; j < 4; ++j) {
            uint32_t rgb = out[i].vertices[j].rgb;
            out[i].vertices[j].rgb = (rgb >> 16) | (rgb & 0xff00) | ((rgb & 0xff) << 16);
        }
    *has_blade = 0;
    if (!root) return count;
    int32_t eye[3];
    for (unsigned i = 0; i < 3; ++i) {
        int64_t dot = 0;
        for (unsigned j = 0; j < 3; ++j)
            dot += (int64_t)camera->rotation[j][i] * camera->translation[j];
        eye[i] = (int32_t)(-dot / 4096);
    }
    if ((visible & (1u << 22)) && fusion) {
        blade->tick = tick;
        *has_blade = bfm_forest_blade_transform(fusion, root, eye, &blade->world);
    }
    if (visible & (1u << 25))
        count += bfm_lumina_outline_glow(effects->lumina_rim, lumina, root, eye, camera, tick,
                                         out + count, BFM_STEAM_WEAPON_CARDS - count);
    return count;
}
