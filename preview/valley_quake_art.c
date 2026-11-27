/* SPDX-License-Identifier: MIT */
#include "valley_quake_art.h"
#include "woods_fusion_art.h"

#include <math.h>

enum { UNIT = 4096, SECTORS = 12, PRIMARY_LIFE = 16, ECHO_LIFE = 32, SCANOUT_DELAY = 2 };

static int echo_start(unsigned index)
{
    return index == 1 ? 1 : 9;
}

int bfm_valley_quake_art_step(BfmValleyQuakeArt *art, int quake,
                              const int16_t ground[3], unsigned yaw)
{
    if (!art) return 0;
    if (quake && ground) {
        *art = (BfmValleyQuakeArt){.ground = {ground[0], ground[1], ground[2]},
                                   .yaw = yaw, .age = 1, .scale = {UNIT}};
        return 0;
    }
    if (!art->age) return 0;

    int tick = (int)art->age++;
    int child = tick == echo_start(1) || tick == echo_start(2);
    /* The US scanout trails the grounded pose by two field updates. Audio
     * follows the allocation on its simulation update. */
    tick -= SCANOUT_DELAY;
    if (tick > echo_start(2) + ECHO_LIFE) {
        art->age = 0;
        return 0;
    }
    /* MAIN/43 moves each radius one tenth of the way to five times its
     * original size per update. The secondary rings grow from a point. */
    for (unsigned i = 0; i < 3; ++i) {
        int start = i ? echo_start(i) : 0;
        int life = i ? ECHO_LIFE : PRIMARY_LIFE;
        if (tick > start && tick <= start + life)
            art->scale[i] += (5 * UNIT - art->scale[i]) / 10;
    }
    return child;
}

static int q12(int value, int scale)
{
    int product = value * scale;
    return product / UNIT - (product % UNIT < 0);
}

static void outline(int16_t rim[SECTORS][3])
{
    /* The procedural pad uses one quantized quadrant and reflects it, keeping
     * opposite edges symmetric despite the integer angle step. */
    for (unsigned i = 0; i <= 3; ++i) {
        double angle = i * (341 * 6.28318530717958647693 / UNIT);
        rim[i][0] = (int16_t)(lround(UNIT * sin(angle)) / 64);
        rim[i][1] = (int16_t)(lround(UNIT * cos(angle)) / 64);
    }
    for (unsigned i = 4; i <= 6; ++i) {
        rim[i][0] = rim[6-i][0];
        rim[i][1] = -rim[6-i][1];
    }
    for (unsigned i = 7; i < SECTORS; ++i) {
        rim[i][0] = -rim[SECTORS-i][0];
        rim[i][1] = rim[SECTORS-i][1];
    }
}

static int draw_wave(BfmBedroomFigure *figure, const BfmBedroomView *view,
                     const BfmForestCamera *camera, const BfmValleyQuakeArt *art,
                     unsigned i, const int16_t rim[SECTORS][3])
{
    int tick = (int)art->age - 1 - SCANOUT_DELAY;
    int age = i ? tick - echo_start(i) : tick;
    if (age <= 0 || age > (i ? ECHO_LIFE : PRIMARY_LIFE)) return 1;

    int fraction, rim_light, fill_light;
    int16_t point[3] = {art->ground[0], art->ground[1], art->ground[2]};
    if (i) {
        /* Allocation and first update of the child both happen after the
         * primary timer has decremented. Its captured value controls its
         * color and the height of its shallow arc. */
        int strength = PRIMARY_LIFE - echo_start(i);
        int angle = art->scale[i] / 10;
        int wave = (int)lround(UNIT * sin(angle * (6.28318530717958647693 / UNIT)));
        point[1] -= (2 * strength * wave) / UNIT;
        fraction = 3276;
        rim_light = 12 * strength;
        fill_light = 0;
    } else {
        fraction = 16 * age * (age + 1);
        if (fraction > UNIT) fraction = UNIT;
        fill_light = 8 * (PRIMARY_LIFE + 1 - age);
        rim_light = 0;
    }

    BfmForestTransform model;
    bfm_forest_actor_pitch_yaw(1024, art->yaw, point, &model);
    for (unsigned row = 0; row < 3; ++row)
        for (unsigned col = 0; col < 2; ++col)
            model.rotation[row][col] = (int16_t)q12(model.rotation[row][col], art->scale[i]);
    BfmForestCamera composed;
    if (!bfm_forest_effect_camera(camera, &model, &composed)) return 0;
    BfmBedroomView local = {.focal = composed.projection};
    for (unsigned row = 0; row < 3; ++row) {
        local.translation[row] = composed.translation[row];
        for (unsigned col = 0; col < 3; ++col) local.axes[row][col] = composed.rotation[row][col];
    }
    const int16_t origin[3] = {0};
    int center[3];
    if (!bfm_bedroom_project(&local, origin, center) || center[2] / 4 + 1 >= 4096) return 1;
    unsigned bucket = (unsigned)center[2] / 4 + 1;
    BfmForestRasterVertex outer[SECTORS], inner[SECTORS];
    for (unsigned k = 0; k < SECTORS; ++k) {
        int screen[3];
        bfm_bedroom_project(&local, rim[k], screen);
        outer[k] = (BfmForestRasterVertex){.x = screen[0], .y = screen[1], .rgb = rim_light * 257u};
        inner[k] = (BfmForestRasterVertex){.x = center[0] + q12(screen[0]-center[0], fraction),
                                          .y = center[1] + q12(screen[1]-center[1], fraction)};
    }
    BfmForestDustCard cards[SECTORS];
    BfmForestRasterMaterial material = {.page = 32, .count = 4, .translucent = 1};
    if (rim_light) {
        for (unsigned k = 0; k < SECTORS; ++k) {
            unsigned next = (k+1) % SECTORS;
            cards[k] = (BfmForestDustCard){.bucket = bucket, .material = material,
                .vertices = {outer[k], outer[next], inner[k], inner[next]}};
        }
        if (!bfm_bedroom_figure_add_fusion(figure, view, NULL, cards, SECTORS)) return 0;
    }
    if (fill_light) {
        for (unsigned k = 0; k < SECTORS; ++k) inner[k].rgb = fill_light * 257u;
        BfmForestRasterVertex hub = {.x = center[0], .y = center[1]};
        for (unsigned k = 0; k < SECTORS; k += 2) {
            cards[k/2] = (BfmForestDustCard){.bucket = bucket, .material = material,
                .vertices = {inner[k], inner[k+1], hub, inner[(k+2)%SECTORS]}};
        }
        if (!bfm_bedroom_figure_add_fusion(figure, view, NULL, cards, SECTORS/2)) return 0;
    }
    return 1;
}

int bfm_valley_quake_art_draw(const BfmValleyQuakeArt *art, BfmBedroomFigure *figure,
                              const BfmBedroomView *view)
{
    if (!art || !figure || !view) return 0;
    if (!art->age) return 1;
    BfmForestCamera camera;
    if (!bfm_woods_effect_camera(view, &camera)) return 0;
    int16_t rim[SECTORS][3] = {{0}};
    outline(rim);
    for (unsigned i = 0; i < 3; ++i)
        if (!draw_wave(figure, view, &camera, art, i, rim)) return 0;
    return 1;
}
