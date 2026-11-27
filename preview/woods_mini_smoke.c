/* SPDX-License-Identifier: MIT */
#include "woods_mini_smoke.h"
#include "forest_bullet_art.h"

#include <limits.h>

static int floor_div(int64_t value, int divisor)
{
    return (int)(value / divisor - (value % divisor < 0));
}

int bfm_woods_mini_smoke_load(BfmWoodsMiniSmoke *smoke, const uint8_t *main_data, size_t bytes)
{
    BfmWoodsMiniSmoke loaded = {0};
    if (!smoke || !bfm_hero_run_smoke_load(&loaded.material, main_data, bytes)) return 0;
    *smoke = loaded;
    return 1;
}

void bfm_woods_mini_smoke_step(BfmWoodsMiniSmoke *smoke, int applied_mini,
                              const int16_t feet[3], const int eye[3])
{
    if (!smoke || !smoke->material.width) return;
    if (!smoke->age) {
        if (!applied_mini || !feet || !eye) return;
        int32_t center[3] = {feet[0], feet[1] - 16, feet[2]};
        BfmForestTransform shifted;
        bfm_forest_glow_facing(eye, center, 6, &shifted);
        smoke->x = (int16_t)shifted.translation[0];
        smoke->z = (int16_t)shifted.translation[2];
        smoke->y_q16 = (int64_t)shifted.translation[1] * 65536;
        smoke->rise_q16 = -8 * 65536;
    }
    if (!bfm_hero_smoke_frame(&smoke->material, 0, ++smoke->age)) {
        smoke->age = 0;
        return;
    }
    smoke->y_q16 += smoke->rise_q16;
    smoke->rise_q16 -= floor_div(smoke->rise_q16, 8);
}

int bfm_woods_mini_smoke_add(const BfmWoodsMiniSmoke *smoke, BfmBedroomFigure *figure,
                            const BfmBedroomView *view)
{
    if (!smoke || !figure || !view || view->focal <= 0 || view->focal > INT16_MAX) return 0;
    if (!smoke->age) return 1;
    const BfmHeroSmokeFrame *frame = bfm_hero_smoke_frame(&smoke->material, 0, smoke->age);
    if (!frame) return 0;
    int16_t world[3] = {smoke->x, (int16_t)floor_div(smoke->y_q16, 65536), smoke->z};
    int center[3];
    if (!bfm_bedroom_project(view, world, center) || center[2] / 4 + 1 >= 4096) return 1;

    /* Use the sprite center's camera depth for both local projections. The
     * GPU adds the projected extent to the top-left after rounding each. */
    BfmBedroomView local = {.axes = {{12288,0,0}, {0,12288,0}, {0,0,4096}},
                            .focal = view->focal};
    int16_t offset[3] = {smoke->material.left, smoke->material.top, (int16_t)center[2]};
    int16_t extent[3] = {smoke->material.width, smoke->material.height, offset[2]};
    int origin[3], end[3];
    bfm_bedroom_project(&local, offset, origin);
    bfm_bedroom_project(&local, extent, end);
    int x = center[0] + origin[0] - 160, y = center[1] + origin[1] - 120;
    BfmForestDustCard card = {
        .bucket = (unsigned)center[2] / 4 + 1,
        .material = {.page = frame->page, .clut = frame->palette, .count = 4,
                     .textured = 1, .translucent = 1}
    };
    for (unsigned i = 0; i < 4; ++i) {
        card.vertices[i] = (BfmForestRasterVertex){
            .x = x + (i & 1 ? end[0] - 160 : 0), .y = y + (i & 2 ? end[1] - 120 : 0),
            .u = frame->uv[i][0], .v = frame->uv[i][1], .rgb = 0x80f080
        };
    }
    return bfm_bedroom_figure_add_fusion(figure, view, NULL, &card, 1);
}
