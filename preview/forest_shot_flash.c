/* SPDX-License-Identifier: MIT */
#include "forest_shot_flash.h"
#include <math.h>

static int64_t floor_div(int64_t value, int divisor)
{
    return value / divisor - (value % divisor < 0);
}

static int short_value(uint32_t value)
{
    return (int32_t)((value & 65535u) ^ 32768u) - 32768;
}

/* Contact debris begins a short distance toward the eye. The field reduces
 * squared distance to a base-four mantissa before normalizing its direction. */
static void shift_toward_eye(const int32_t eye[3], int16_t point[3])
{
    int difference[3];
    unsigned square = 0;
    for (unsigned k = 0; k < 3; ++k) {
        int d = short_value((uint32_t)eye[k] - (uint16_t)point[k]);
        if (d < -16383 || d > 16383) {
            point[1] = (int16_t)short_value((uint16_t)point[1] - 65u);
            return;
        }
        difference[k] = d;
        square += (unsigned)(d * d);
    }
    if (!square) return;
    int divisor = 512;
    while (square < 64) { square *= 4; divisor /= 2; }
    while (square > 255) { square /= 4; divisor *= 2; }
    int gain = (int)sqrt((double)(1u << 30) / square);
    for (unsigned k = 0; k < 3; ++k) {
        int offset = (int)floor_div((int64_t)difference[k] * gain, divisor);
        point[k] = (int16_t)short_value((uint32_t)point[k] + (uint32_t)offset);
    }
}

static void rise(BfmForestShotFlash *flash)
{
    flash->height_q16 += (uint32_t)flash->velocity_q16;
    flash->velocity_q16 -= (int32_t)floor_div(flash->velocity_q16, 8);
}

int bfm_forest_shot_flash_start(BfmForestShotFlash *flash, BfmForestShotFlashKind kind,
                                const int16_t origin[3], const int32_t eye[3])
{
    if (!flash || !origin || (kind != BFM_SHOT_FLASH_CONTACT && kind != BFM_SHOT_FLASH_EXPIRY) ||
        (kind == BFM_SHOT_FLASH_CONTACT && !eye))
        return 0;
    int16_t point[3] = {origin[0], origin[1], origin[2]};
    if (kind == BFM_SHOT_FLASH_CONTACT) shift_toward_eye(eye, point);
    *flash = (BfmForestShotFlash){
        .x = point[0], .z = point[2], .height_q16 = (uint32_t)point[1] << 16,
        .velocity_q16 = kind == BFM_SHOT_FLASH_CONTACT ? -5 * 65536 : 0, .cell = 1
    };
    rise(flash);
    return 1;
}

void bfm_forest_shot_flash_advance(BfmForestShotFlash *flash)
{
    if (!flash || !flash->cell) return;
    if (flash->cell >= BFM_FOREST_DUST_CELLS - 1) {
        flash->cell = 0;
        return;
    }
    ++flash->cell;
    rise(flash);
}

int bfm_forest_shot_flash_project(const BfmForestShotFlash *flash, const BfmForestDust *atlas,
                                  const BfmForestCamera *camera, BfmForestDustCard *out)
{
    if (!flash || !flash->cell || flash->cell >= BFM_FOREST_DUST_CELLS || !atlas ||
        !atlas->width || !atlas->height || !camera || !out)
        return 0;
    const int world[3] = {flash->x, short_value(flash->height_q16 >> 16), flash->z};
    int16_t view[3];
    for (unsigned i = 0; i < 3; ++i) {
        int64_t sum = 0;
        for (unsigned k = 0; k < 3; ++k) sum += (int64_t)camera->rotation[i][k] * world[k];
        int64_t v = camera->translation[i] + floor_div(sum, 4096);
        if (v < INT16_MIN || v > INT16_MAX) return 0;
        view[i] = (int16_t)v;
    }
    /* The sprite projects its world center, local pivot and size independently
     * at the same depth. Combining them first changes short-distance rounding. */
    const BfmForestCamera screen = {
        .rotation = {{4096,0,0},{0,4096,0},{0,0,4096}}, .projection = camera->projection
    };
    int pivot_x = (int)floor_div((int)atlas->left * 7, 4);
    int pivot_y = (int)floor_div((int)atlas->top * 7, 4);
    if (pivot_x < INT16_MIN || pivot_x > INT16_MAX ||
        pivot_y < INT16_MIN || pivot_y > INT16_MAX) return 0;
    const int16_t vectors[4][3] = {
        {view[0], view[1], view[2]}, {(int16_t)pivot_x, (int16_t)pivot_y, view[2]},
        {(int16_t)(atlas->width * 7 / 4), (int16_t)(atlas->height * 7 / 4), view[2]},
        {0, 0, view[2]}
    };
    BfmForestDustCard card = {0};
    if (!bfm_forest_project_effect_quad(&screen, vectors, &card) || ++card.bucket >= 4096)
        return 0;
    const BfmForestRasterVertex *v = card.vertices;
    int x = v[0].x + v[1].x - v[3].x, y = v[0].y + v[1].y - v[3].y;
    int width = v[2].x - v[3].x, height = v[2].y - v[3].y;
    if (width <= 0 || height <= 0) return 0;
    card.material = (BfmForestRasterMaterial){
        .page = atlas->page[flash->cell], .clut = (412u << 6) | 352u / 16,
        .count = 4, .textured = 1, .translucent = 1
    };
    for (unsigned i = 0; i < 4; ++i) {
        unsigned right = i & 1, bottom = i / 2;
        card.vertices[i] = (BfmForestRasterVertex){
            x + (int)right * width, y + (int)bottom * height,
            (uint8_t)(atlas->u[flash->cell] + right * (atlas->width - 1)),
            (uint8_t)(atlas->v[flash->cell] + bottom * (atlas->height - 1)), 0x808080
        };
    }
    *out = card;
    return 1;
}
