/* SPDX-License-Identifier: MIT */
#include "tower_press_effect.h"
#include "../pc_port/forest_player_bytes.h"
#include <math.h>

int bfm_tower_press_colors_load(BfmTowerPressColors *colors,
                                const uint8_t *scene, size_t size)
{
    enum { palette = 0x5eea0 };
    if (!colors || !scene || size < palette + sizeof colors->rim) return 0;
    for (unsigned i = 0; i <= BFM_TOWER_PRESS_CARDS; ++i)
        colors->rim[i] = u32le(scene + palette + 4*i) & 0xffffffu;
    return 1;
}

static int sine(unsigned angle)
{
    return (int)lround(4096 * sin((angle & 4095u) * (6.28318530717958647693 / 4096)));
}

static int unscale(int product)
{
    return product / 4096 - (product % 4096 < 0);
}

/* The flare widens from the already quantized lower rim. Its top edge
 * fades to black under additive blending as it rises from the plate. */
size_t bfm_tower_press_cards(const int16_t center[3], unsigned remaining,
                             const BfmForestCamera *camera, const BfmTowerPressColors *colors,
                             BfmForestDustCard out[BFM_TOWER_PRESS_CARDS])
{
    if (!center || !camera || !camera->projection || !colors || !out ||
        !remaining || remaining > 30) return 0;
    int lift = sine(2048 * remaining / 30);
    int stretch = 4096 + 3 * lift;
    int height = unscale(256 * lift);
    int16_t bottom[BFM_TOWER_PRESS_CARDS+1][3], top[BFM_TOWER_PRESS_CARDS+1][3];
    for (unsigned i = 0; i <= BFM_TOWER_PRESS_CARDS; ++i) {
        unsigned angle = 16 * remaining + 682 * i;
        int x = unscale(32 * sine(angle)), z = unscale(32 * sine(angle + 1024));
        int top_x = center[0] + unscale(x * stretch);
        int top_z = center[2] + unscale(z * stretch);
        if (top_x < INT16_MIN || top_x > INT16_MAX || top_z < INT16_MIN ||
            top_z > INT16_MAX || center[1] - height < INT16_MIN) return 0;
        bottom[i][0] = (int16_t)(center[0] + x);
        bottom[i][1] = center[1];
        bottom[i][2] = (int16_t)(center[2] + z);
        top[i][0] = (int16_t)top_x;
        top[i][1] = (int16_t)(center[1] - height);
        top[i][2] = (int16_t)top_z;
    }

    size_t count = 0;
    for (unsigned i = 0; i < BFM_TOWER_PRESS_CARDS; ++i) {
        /* The sorter uses the first rim vertex. Project it last to obtain
         * its bucket, then restore the original four-corner strip order. */
        int16_t corners[4][3];
        for (unsigned axis = 0; axis < 3; ++axis) {
            corners[0][axis] = top[i+1][axis];
            corners[1][axis] = top[i][axis];
            corners[2][axis] = bottom[i+1][axis];
            corners[3][axis] = bottom[i][axis];
        }
        BfmForestDustCard projected = {0};
        if (!bfm_forest_project_effect_quad(camera, corners, &projected)) continue;
        BfmForestDustCard card = {
            .vertices={projected.vertices[3], projected.vertices[2],
                       projected.vertices[1], projected.vertices[0]},
            .material={.count=4, .page=32, .translucent=1},
            .bucket=projected.bucket
        };
        card.vertices[0].rgb = colors->rim[i];
        card.vertices[1].rgb = colors->rim[i+1];
        out[count++] = card;
    }
    return count;
}
