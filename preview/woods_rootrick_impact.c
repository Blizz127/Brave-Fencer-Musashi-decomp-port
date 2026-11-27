/* SPDX-License-Identifier: MIT */
#include "woods_rootrick_impact.h"
#include "woods_fusion_art.h"
#include "../pc_port/forest_player_bytes.h"

#include <limits.h>

int bfm_woods_rootrick_impact_load(BfmWoodsRootrickImpact *out, const uint8_t *scene,
                                   size_t bytes, const uint8_t *main, size_t main_bytes)
{
    const size_t header = 0x80190ce4u - 0x80128158u;
    const size_t shake = 0x80195184u - 0x80128158u;
    if (!out || !scene || bytes < shake + 18 || u32le(scene + header) != UINT32_C(0x801961e8))
        return 0;
    BfmWoodsRootrickImpact next = {.shake = scene + shake};
    for (unsigned i = 0; i < 8; ++i)
        if (!next.shake[2 * i + 1]) return 0;
    if (next.shake[17] || !bfm_hero_run_smoke_load(&next.smoke, main, main_bytes)) return 0;
    *out = next;
    return 1;
}

void bfm_woods_rootrick_impact_shake(BfmWoodsRootrickImpact *impact)
{
    if (!impact || !impact->shake) return;
    impact->next_sample = 0;
    impact->hold = 0;
    impact->shaking = 1;
}

void bfm_woods_rootrick_impact_hit(BfmWoodsRootrickImpact *impact, const int16_t xyz[3])
{
    if (!impact || !impact->shake || !xyz) return;
    impact->center[0] = xyz[0];
    impact->center[1] = (int16_t)(xyz[1] + 40);
    impact->center[2] = (int16_t)(xyz[2] + 82);
    impact->puff_age = 0;
    impact->puffs_active = 1;
    bfm_woods_rootrick_impact_shake(impact);
}

void bfm_woods_rootrick_impact_tick(BfmWoodsRootrickImpact *impact)
{
    if (!impact || !impact->shake) return;
    if (impact->puffs_active && !bfm_hero_smoke_frame(&impact->smoke, 0, ++impact->puff_age))
        impact->puffs_active = 0;
    impact->camera_y = impact->next_camera_y;
    if (!impact->shaking) return;
    if (impact->hold) {
        --impact->hold;
        return;
    }
    const uint8_t *sample = impact->shake + 2 * impact->next_sample++;
    impact->hold = sample[1];
    impact->shaking = impact->hold != 0;
    impact->next_camera_y = impact->shaking ? (int)(sample[0] ^ 128u) - 128 : 0;
}

int bfm_woods_rootrick_impact_camera_y(const BfmWoodsRootrickImpact *impact)
{
    return impact ? impact->camera_y : 0;
}

int bfm_woods_rootrick_impact_submit(const BfmWoodsRootrickImpact *impact,
                                     BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    if (!impact || !figure || !view) return 0;
    if (!impact->puffs_active) return 1;
    const BfmHeroSmokeMaterial *smoke = &impact->smoke;
    const BfmHeroSmokeFrame *frame = bfm_hero_smoke_frame(smoke, 0, impact->puff_age);
    if (!frame) return 0;
    BfmForestCamera camera, centered;
    /* The sprite uses the transformed center and faces the camera. */
    BfmForestTransform particle = {.translation = {
        impact->center[0], impact->center[1], impact->center[2]
    }};
    if (!bfm_woods_effect_camera(view, &camera)) return 0;
    if (!bfm_forest_effect_camera(&camera, &particle, &centered)) return 1;
    const int32_t *center = centered.translation;
    for (unsigned axis = 0; axis < 3; ++axis)
        if (center[axis] < INT16_MIN || center[axis] > INT16_MAX) return 1;

    BfmForestCamera plane = {.rotation = {{4096,0,0},{0,4096,0},{0,0,4096}},
                             .projection = camera.projection};
    BfmForestDustCard cards[2] = {0};
    for (unsigned i = 0; i < 2; ++i) {
        int scale = 3 * (int)(i + 1);
        int16_t z = (int16_t)center[2];
        /* The sprite projects its world center, signed offset and extent at the
         * same depth. Round each independently before constructing the FT4. */
        int16_t points[4][3] = {
            {(int16_t)center[0], (int16_t)center[1], z},
            {(int16_t)(scale * smoke->left), (int16_t)(scale * smoke->top), z},
            {(int16_t)(scale * smoke->width), (int16_t)(scale * smoke->height), z},
            {0, 0, z}
        };
        BfmForestDustCard projected;
        if (!bfm_forest_project_effect_quad(&plane, points, &projected) ||
            projected.bucket + 1 >= 4096) return 1;
        const BfmForestRasterVertex *v = projected.vertices;
        int left = v[0].x + v[1].x - v[3].x, top = v[0].y + v[1].y - v[3].y;
        int width = v[2].x - v[3].x, height = v[2].y - v[3].y;
        BfmForestDustCard *card = &cards[i];
        card->bucket = projected.bucket + 1;
        card->material = (BfmForestRasterMaterial){.count = 4, .textured = 1, .translucent = 1,
                                                   .page = frame->page, .clut = frame->palette};
        for (unsigned k = 0; k < 4; ++k) {
            card->vertices[k] = (BfmForestRasterVertex){
                .x = left + ((k & 1) ? width : 0), .y = top + ((k & 2) ? height : 0),
                .u = frame->uv[k][0], .v = frame->uv[k][1], .rgb = 0x808080
            };
        }
    }
    return bfm_bedroom_figure_add_fusion(figure, view, NULL, cards, 2);
}
