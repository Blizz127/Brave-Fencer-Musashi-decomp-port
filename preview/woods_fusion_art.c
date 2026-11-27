/* SPDX-License-Identifier: MIT */
#include "woods_fusion_art.h"
#include "woods_flight_glow.h"

int bfm_woods_fusion_art_pose(const uint8_t *main, size_t main_bytes, uint32_t pose,
                              uint32_t visible, int blade_away, BfmBedroomPose *body)
{
    if (!body) return 0;
    if (pose) {
        const uint32_t main_base = UINT32_C(0x800cedf8);
        if (!main || (pose & 3) || pose < main_base + 16) return 0;
        size_t offset = pose - main_base - 16;
        if (offset > main_bytes || main_bytes - offset < 256) return 0;
        body->pose = main + offset;
        body->show_weapon = (visible >> 22) & 1u;
    }
    body->hide_fusion = !!blade_away;
    return 1;
}

static int effect_camera(const BfmBedroomView *view, BfmForestCamera *camera)
{
    if (view->focal <= 0 || view->focal > UINT16_MAX) return 0;
    camera->projection = (uint16_t)view->focal;
    for (unsigned i = 0; i < 3; ++i) {
        for (unsigned j = 0; j < 3; ++j) {
            int axis = view->axes[i][j];
            if (axis < INT16_MIN || axis > INT16_MAX) return 0;
            camera->rotation[i][j] = (int16_t)axis;
        }
        camera->translation[i] = view->translation[i];
    }
    return 1;
}

int bfm_woods_fusion_art_append(BfmBedroomFigure *figure, const BfmBedroomView *view,
                                const BfmForestActor *blade, const BfmForestTetherTrails *trails,
                                const BfmForestDamageDigits *bp_digits)
{
    if (!figure || !view) return 0;
    if ((!trails || !trails->enabled) && (!bp_digits || !bp_digits->amount))
        return !blade || bfm_bedroom_figure_add_fusion(figure, view, blade, NULL, 0);
    BfmForestCamera camera;
    if (!effect_camera(view, &camera)) return 0;
    BfmForestDustCard cards[BFM_TETHER_CARDS + BFM_FOREST_DAMAGE_DIGIT_CARDS];
    size_t count = bfm_forest_tether_trails_project(trails, &camera, cards, BFM_TETHER_CARDS);
    count += bfm_forest_damage_digits_project(bp_digits, &camera, cards + count);
    return bfm_bedroom_figure_add_fusion(figure, view, blade, cards, count);
}

int bfm_woods_fusion_art_lights(BfmBedroomFigure *figure, const BfmBedroomView *view,
                                const BfmWoodsFusionFrame *frame)
{
    if (!figure || !view || !frame) return 0;
    if (!frame->glow) return 1;
    BfmForestCamera camera;
    if (!effect_camera(view, &camera)) return 0;
    BfmForestDustCard cards[BFM_WOODS_HALO_CARDS + BFM_WOODS_BLADE_SPARK_CARDS +
                            BFM_WOODS_CAPTURE_LIGHT_CARDS];
    size_t count = bfm_woods_flight_glow_project(frame->glow, frame->flight, &camera, view->eye,
                                                 cards, sizeof cards / sizeof cards[0]);
    return !count || bfm_bedroom_figure_add_fusion(figure, view, NULL, cards, count);
}
