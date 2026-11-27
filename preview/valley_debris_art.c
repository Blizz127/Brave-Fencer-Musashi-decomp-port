/* SPDX-License-Identifier: MIT */
#include "valley_debris_art.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>

int bfm_valley_debris_art_load(BfmValleyDebrisArt *art, const uint8_t *scene, size_t bytes)
{
    const size_t shape_offset = 0x80186194u - 0x80128158u;
    const size_t frames_offset = 0x801861a0u - 0x80128158u;
    if (!art || !scene || bytes < frames_offset + 8 * BFM_VALLEY_DEBRIS_CELS) return 0;

    const uint8_t *shape = scene + shape_offset;
    BfmValleyDebrisArt next = {
        .width = shape[2], .height = shape[3], .left = s16le(shape + 8), .top = s16le(shape + 10)
    };
    if (shape[0] != 255 || !next.width || !next.height ||
        3 * next.left < INT16_MIN || 3 * next.left > INT16_MAX ||
        3 * next.top < INT16_MIN || 3 * next.top > INT16_MAX) return 0;
    int dx = s16le(shape + 4);
    int word_offset = dx / 4 - (dx % 4 < 0);
    for (unsigned i = 0; i < BFM_VALLEY_DEBRIS_CELS; ++i) {
        const uint8_t *frame = scene + frames_offset + 8 * i;
        int x = u16le(frame) + word_offset;
        int y = u16le(frame + 2) + u16le(shape + 6);
        unsigned timing = u32le(frame + 4);
        if (x < 0 || x >= 1024 || y >= 512 || !(timing & 63u) ||
            (timing & ~63u) != (i + 1 == BFM_VALLEY_DEBRIS_CELS ? 192u : 0u)) return 0;
        next.duration[i] = (uint8_t)(timing & 63u);
        next.frame[i].u = (uint8_t)(4 * (x % 64));
        next.frame[i].v = (uint8_t)(y % 256);
        /* The effect uses 4-bit texels and adds one quarter of the foreground. */
        next.frame[i].page = (uint16_t)(x / 64 | (y / 256) << 4 | 3u << 5);
    }
    *art = next;
    return 1;
}

int bfm_valley_debris_art_project_scaled(const BfmValleyDebrisArt *art, unsigned cel, unsigned scale,
    const int16_t xyz[3], const BfmForestCamera *camera, unsigned slot, BfmForestDustCard *out)
{
    if (!art || cel >= BFM_VALLEY_DEBRIS_CELS || !art->duration[cel] ||
        !xyz || !camera || !out || scale < 2 || scale > 5) return 0;
    int size = (int)scale;
    if (size * art->left < INT16_MIN || size * art->left > INT16_MAX ||
        size * art->top < INT16_MIN || size * art->top > INT16_MAX) return 0;
    BfmForestTransform anchor = {.translation = {xyz[0], xyz[1], xyz[2]}};
    BfmForestCamera center;
    if (!bfm_forest_effect_camera(camera, &anchor, &center)) return 0;
    for (unsigned i = 0; i < 3; ++i)
        if (center.translation[i] < INT16_MIN || center.translation[i] > INT16_MAX) return 0;

    /* Project the center, signed hotspot and full extent independently. The sprite size
     * changes the camera-plane rectangle, leaving the texel extent unchanged. */
    int16_t z = (int16_t)center.translation[2];
    int16_t points[4][3] = {
        {(int16_t)center.translation[0], (int16_t)center.translation[1], z},
        {(int16_t)(size * art->left), (int16_t)(size * art->top), z},
        {(int16_t)(size * art->width), (int16_t)(size * art->height), z}, {0, 0, z}
    };
    BfmForestCamera plane = {.rotation={{4096,0,0},{0,4096,0},{0,0,4096}},
                             .projection=camera->projection};
    BfmForestDustCard projected = {0};
    if (!bfm_forest_project_effect_quad(&plane, points, &projected) || ++projected.bucket >= 4096)
        return 0;
    const BfmForestRasterVertex *v = projected.vertices;
    int x = v[0].x + v[1].x - v[3].x, y = v[0].y + v[1].y - v[3].y;
    int width = v[2].x - v[3].x, height = v[2].y - v[3].y;
    projected.slot = slot;
    projected.material = (BfmForestRasterMaterial){
        .page = art->frame[cel].page, .clut = (357u << 6) | (352u / 16),
        .count = 4, .textured = 1, .translucent = 1
    };
    for (unsigned i = 0; i < 4; ++i) {
        projected.vertices[i] = (BfmForestRasterVertex){
            .x = x + ((i & 1) ? width : 0), .y = y + ((i & 2) ? height : 0),
            .u = (uint8_t)(art->frame[cel].u + ((i & 1) ? art->width - 1 : 0)),
            .v = (uint8_t)(art->frame[cel].v + ((i & 2) ? art->height - 1 : 0)),
            .rgb = UINT32_C(0x808080)
        };
    }
    *out = projected;
    return 1;
}

int bfm_valley_debris_art_project(const BfmValleyDebrisArt *art, unsigned cel,
    const int16_t xyz[3], const BfmForestCamera *camera, unsigned slot, BfmForestDustCard *out)
{
    return bfm_valley_debris_art_project_scaled(art, cel, 3, xyz, camera, slot, out);
}
