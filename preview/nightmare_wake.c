/* SPDX-License-Identifier: MIT */
#include "nightmare_wake.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>

static int64_t floor_q12(int64_t value)
{
    return value / 4096 - (value % 4096 < 0);
}

int bfm_nightmare_wake_start(BfmNightmareWake *effect, const uint8_t *scene,
                             size_t bytes, uint32_t address, const int16_t player[3])
{
    if (address < 0x80128158u) return 0;
    size_t shape_offset = address - 0x80128158u;
    if (!effect || !scene || !player || shape_offset > bytes || bytes - shape_offset < 12) return 0;
    const uint8_t *shape = scene + shape_offset;
    int texel_x = s16le(shape + 4);
    int x = 656 + texel_x/4 - (texel_x%4 < 0);
    unsigned y = 256u + u16le(shape + 6);
    int left = s16le(shape + 8), top = s16le(shape + 10);
    if (shape[0] != 255 || !shape[2] || !shape[3] || x < 0 || x >= 1024 || y >= 512 ||
        4*(x%64) + shape[2] > 256 || y%256 + shape[3] > 256 ||
        left < INT16_MIN/2 || left > INT16_MAX/2 ||
        top < INT16_MIN/2 || top > INT16_MAX/2) return 0;

    *effect = (BfmNightmareWake){
        .position={(int16_t)(player[0]-64), (int16_t)(player[1]-16), (int16_t)(player[2]+16)},
        .left=(int16_t)left, .top=(int16_t)top,
        .page=(uint16_t)(32u | (unsigned)x/64 | (y/256)<<4),
        .u=(uint8_t)(4*(x%64)), .v=(uint8_t)(y%256),
        .width=shape[2], .height=shape[3], .remaining=4
    };
    return 1;
}

void bfm_nightmare_wake_step(BfmNightmareWake *effect)
{
    if (effect && effect->remaining) --effect->remaining;
}

int bfm_nightmare_wake_card(const BfmNightmareWake *effect, const BfmBedroomView *view,
                            BfmForestDustCard *out)
{
    if (!effect || !view || !out || !effect->remaining || effect->remaining == 2 ||
        view->focal <= 0 || view->focal > UINT16_MAX) return 0;

    /* Project the pivot and dimensions independently at the center's depth.
     * Rotation acts on the resulting integer screen corners. */
    int16_t points[4][3] = {{0}};
    for (unsigned row = 0; row < 3; ++row) {
        int64_t dot = 0;
        for (unsigned col = 0; col < 3; ++col)
            dot += (int64_t)view->axes[row][col] * effect->position[col];
        int64_t coord = view->translation[row] + floor_q12(dot);
        if (coord < INT16_MIN || coord > INT16_MAX) return 0;
        points[0][row] = (int16_t)coord;
    }
    points[1][0] = (int16_t)(2 * effect->left);
    points[1][1] = (int16_t)(2 * effect->top);
    points[2][0] = (int16_t)(2 * effect->width);
    points[2][1] = (int16_t)(2 * effect->height);
    for (unsigned i = 1; i < 4; ++i) points[i][2] = points[0][2];
    const BfmForestCamera plane = {.rotation={{4096,0,0},{0,4096,0},{0,0,4096}},
                                   .projection=(uint16_t)view->focal};
    BfmForestDustCard card = {0};
    if (!bfm_forest_project_effect_quad(&plane, points, &card) || ++card.bucket >= 4096) return 0;
    int cx=card.vertices[0].x, cy=card.vertices[0].y;
    int left=card.vertices[1].x-card.vertices[3].x, top=card.vertices[1].y-card.vertices[3].y;
    int width=card.vertices[2].x-card.vertices[3].x, height=card.vertices[2].y-card.vertices[3].y;

    BfmForestTransform roll;
    bfm_forest_actor_root((unsigned)-227, effect->position, &roll);
    int cosine = roll.rotation[0][0], sine = roll.rotation[0][2];
    card.material = (BfmForestRasterMaterial){.page=effect->page, .clut=(356u<<6) | (352u/16),
                                              .count=4, .textured=1};
    for (unsigned i = 0; i < 4; ++i) {
        int dx = left + (int)(i%2)*width, dy = top + (int)(i/2)*height;
        card.vertices[i] = (BfmForestRasterVertex){
            .x=cx + (int)floor_q12(cosine*dx - sine*dy),
            .y=cy + (int)floor_q12(sine*dx + cosine*dy),
            .u=(uint8_t)(effect->u + (i%2)*(effect->width-1)),
            .v=(uint8_t)(effect->v + (i/2)*(effect->height-1)), .rgb=UINT32_C(0x808080)
        };
    }
    *out = card;
    return 1;
}
