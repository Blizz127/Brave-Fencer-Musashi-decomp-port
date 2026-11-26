/* SPDX-License-Identifier: MIT */
#include "forest_bullet_art.h"
#include "forest_shore_motion.h"

#include <math.h>
#include <string.h>

int bfm_forest_bullet_art_load(BfmForestBulletArt *art, const BfmForestAward *award)
{
    const uint8_t *shape = bfm_forest_gunshot_bytes(award, UINT32_C(0x800cbb44),
                                                       BFM_GUNSHOT_PERIMETER_BYTES);
    if (!art || !shape || memcmp(shape, shape + 16, 2)) return 0;
    BfmForestBulletArt loaded = {.loaded = 1};
    memcpy(loaded.perimeter, shape, sizeof loaded.perimeter);
    *art = loaded;
    return 1;
}

static int down(int value, int divisor)
{
    return value / divisor - (value % divisor < 0);
}

static int bearing(int a, int b)
{
    int angle = (int)bfm_forest_shore_bearing(a, b);
    return angle > 2048 ? angle - 4096 : angle;
}

static void wave(int angle, int *sine, int *cosine)
{
    const int16_t origin[3] = {0};
    BfmForestTransform root;
    bfm_forest_actor_root((unsigned)(angle < 0 ? -angle : angle), origin, &root);
    *sine = angle < 0 ? -root.rotation[0][2] : root.rotation[0][2];
    *cosine = root.rotation[0][0];
}

void bfm_forest_bullet_art_fire(BfmForestBulletArt *art, const int16_t muzzle[3],
                                const int32_t eye[3])
{
    if (!art || !art->loaded || !muzzle || !eye) return;
    int toward[3] = {0};
    unsigned d2 = 0;
    for (unsigned k = 0; k < 3; ++k) {
        int64_t coordinate = (int64_t)eye[k] - muzzle[k];
        if (coordinate < -16383 || coordinate > 16383) {
            toward[0] = toward[2] = 0;
            toward[1] = -4095;
            d2 = 4095u * 4095u;
            break;
        }
        toward[k] = (int)coordinate;
        d2 += (unsigned)(toward[k] * toward[k]);
    }
    if (d2) {
        unsigned divisor = 8;
        while (d2 > 255) { d2 >>= 2; divisor *= 2; }
        while (d2 < 64) { d2 <<= 2; divisor /= 2; }
        int gain = (int)sqrt((double)(UINT32_C(1) << 30) / d2);
        for (unsigned k = 0; k < 3; ++k) toward[k] = down(toward[k]*gain, (int)divisor);
    }
    int sine_p, cos_p, sine_y, cos_y;
    int horizontal = (int)hypot(toward[0], toward[2]);
    wave(-bearing(toward[1], horizontal), &sine_p, &cos_p);
    wave(bearing(toward[0], toward[2]), &sine_y, &cos_y);
    art->flash = (BfmForestTransform){.rotation = {
        {cos_y, down(sine_y*sine_p,4096), down(sine_y*cos_p,4096)},
        {0, cos_p, -sine_p},
        {-sine_y, down(cos_y*sine_p,4096), down(cos_y*cos_p,4096)}
    }};
    for (unsigned k = 0; k < 3; ++k)
        art->flash.translation[k] = muzzle[k] + down(toward[k], 128);
    art->brightness = 192;
}

void bfm_forest_bullet_art_tick(BfmForestBulletArt *art)
{
    if (art) art->brightness = art->brightness == 192 ? 96 : 0;
}

size_t bfm_forest_bullet_fan_project(const uint8_t perimeter[BFM_GUNSHOT_PERIMETER_BYTES],
                    const BfmForestTransform *root, int plane_xz, uint32_t rgb,
                    const BfmForestCamera *camera, BfmForestDustCard *cards, size_t capacity)
{
    BfmForestCamera local;
    if (!perimeter || !cards || !capacity || !bfm_forest_effect_camera(camera, root, &local))
        return 0;
    size_t count = 0;
    for (unsigned face = 0; face < BFM_GUNSHOT_FAN_CARDS && count < capacity; ++face) {
        int16_t xyz[4][3] = {{0}};
        for (unsigned point = 0; point < 3; ++point) {
            unsigned index = (2 * face + point) * 2;
            unsigned v = point ? point + 1 : 0;
            int a = perimeter[index], b = perimeter[index + 1];
            xyz[v][0] = (int16_t)(a < 128 ? a : a - 256);
            xyz[v][plane_xz ? 2 : 1] = (int16_t)(b < 128 ? b : b - 256);
        }
        BfmForestDustCard card = {.material = {.page=32, .count=4, .translucent=1}};
        card.vertices[1].rgb = rgb;
        if (bfm_forest_project_effect_quad(&local, xyz, &card)) cards[count++] = card;
    }
    return count;
}

size_t bfm_forest_bullet_art_project(const BfmForestBulletArt *art,
                   const BfmForestCamera *camera, BfmForestDustCard *cards, size_t capacity)
{
    if (!art || !art->brightness) return 0;
    unsigned value = art->brightness;
    uint32_t rgb = value | (value << 8) | ((value / 2) << 16);
    return bfm_forest_bullet_fan_project(art->perimeter, &art->flash, 0, rgb,
                                         camera, cards, capacity);
}
