/* SPDX-License-Identifier: MIT */
#include "forest_bank_hop_ring.h"

int bfm_forest_bank_hop_ring_load(BfmForestBankHopRing *effect, const MusashiDiscMedia *disc)
{
    if (!effect || !bfm_woods_foot_dust_load(&effect->dust, disc)) return 0;
    effect->pool = (BfmSteamChargeEffects){0};
    return 1;
}

void bfm_forest_bank_hop_ring_step(BfmForestBankHopRing *effect, const BfmBedroomRoot *root,
                                   const int eye[3], int contact_puff, int launch)
{
    if (!effect || !effect->dust.atlas.width) return;
    if (root && eye) {
        if (contact_puff)
            bfm_woods_foot_dust_emit(&effect->dust, &effect->pool, BFM_FOREST_DUST_BRAKE,
                                     root->xyz, eye, root->yaw);
        if (launch)
            bfm_woods_foot_dust_emit(&effect->dust, &effect->pool, BFM_FOREST_DUST_TRAIL,
                                     root->xyz, eye, root->yaw);
    }
    for (unsigned slot = 0; slot < BFM_STEAM_CHARGE_SLOTS; ++slot)
        bfm_woods_foot_dust_visit(&effect->dust, &effect->pool, slot);
}

int bfm_forest_bank_hop_ring_append(const BfmForestBankHopRing *effect,
                                    BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    if (!effect || !figure || !view || view->focal <= 0 || view->focal > UINT16_MAX) return 0;
    BfmForestCamera camera = {.projection = (uint16_t)view->focal};
    for (unsigned axis = 0; axis < 3; ++axis) {
        for (unsigned j = 0; j < 3; ++j) {
            int value = view->axes[axis][j];
            if (value < INT16_MIN || value > INT16_MAX) return 0;
            camera.rotation[axis][j] = (int16_t)value;
        }
        camera.translation[axis] = view->translation[axis];
    }
    BfmForestDustCard cards[BFM_STEAM_CHARGE_SLOTS];
    unsigned count = bfm_woods_foot_dust_project(&effect->dust, &effect->pool, &camera,
                                                 cards, BFM_STEAM_CHARGE_SLOTS);
    return !count || bfm_bedroom_figure_add_fusion(figure, view, NULL, cards, count);
}
