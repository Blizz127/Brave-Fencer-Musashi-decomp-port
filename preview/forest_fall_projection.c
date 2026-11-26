/* SPDX-License-Identifier: MIT */
#include "forest_fall.h"
#include "forest_pose.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>

static int down(int64_t n, int units)
{
    return (int)(n / units - (n % units < 0));
}

/* The same fixed-precision reciprocal used by the field camera. Normalize the
 * projected depth before refining the reciprocal of its high bits. */
static unsigned perspective(unsigned focal, unsigned depth)
{
    unsigned d = depth;
    while (d < 0x8000u) {
        d *= 2;
        focal *= 2;
    }
    unsigned index = (d - 0x7fc0u) / 128;
    unsigned seed = (0x40000u / (256 + index) + 1) / 2;
    if (seed < 257) seed = 257;
    unsigned correction = (0x2000080u - d * seed) / 256;
    unsigned refined = (correction * seed + 128) / 256;
    return (unsigned)(((uint64_t)focal * refined + 32768) / 65536);
}

static int screen_point(const int view[3], unsigned focal, int *x, int *y)
{
    if (view[0] < INT16_MIN || view[0] > INT16_MAX ||
        view[1] < INT16_MIN || view[1] > INT16_MAX || view[2] <= 0 ||
        view[2] > 65535 || focal >= 2u * (unsigned)view[2])
        return 0;
    int q = (int)perspective(focal, (unsigned)view[2]);
    int sx = down((int64_t)view[0] * q, 65536);
    int sy = down((int64_t)view[1] * q, 65536);
    /* The room discards a projection when its signed screen coordinate
     * saturates. The drawing origin is applied after this check. */
    if (sx < -1024 || sx > 1023 || sy < -1024 || sy > 1023) return 0;
    *x = 160 + sx;
    *y = 120 + sy;
    return q;
}

int bfm_forest_fall_project_sprite(const BfmForestFallSprite *sprite,
                                   const uint8_t *descriptor, const BfmForestCamera *camera,
                                   BfmForestDustCard *out)
{
    if (!sprite || !descriptor || !descriptor[2] || !descriptor[3] ||
        !camera || !out || !camera->projection ||
        (sprite->kind != 1 && sprite->kind != 2)) return 0;
    int center[3];
    for (unsigned row = 0; row < 3; ++row) {
        int64_t dot = 0;
        for (unsigned i = 0; i < 3; ++i) dot += (int64_t)camera->rotation[row][i] * sprite->position[i];
        int64_t axis = (int64_t)camera->translation[row] + down(dot, 4096);
        if (axis < INT16_MIN || axis > INT16_MAX) return 0;
        center[row] = (int)axis;
    }
    int left = s16le(descriptor + 8), top = s16le(descriptor + 10);
    int width = descriptor[2], height = descriptor[3];
    if (sprite->kind == 1) {
        int x, y;
        int q = screen_point(center, camera->projection, &x, &y);
        if (!q) return 0;
        int scale = q / 16;
        x += down(left * scale, 4096);
        y += down(top * scale, 4096);
        for (unsigned i = 0; i < 4; ++i) {
            out->vertices[i].x = x + (int)(i & 1) * (width * scale / 4096);
            out->vertices[i].y = y + (int)(i >> 1) * (height * scale / 4096);
        }
        out->bucket = (unsigned)center[2] / 4 + 1;
    } else {
        /* The second sprite turns about the world's vertical axis. Compose its
         * local right direction with the camera before projecting each corner. */
        BfmForestTransform turn;
        bfm_forest_actor_root(sprite->angle, sprite->position, &turn);
        int right[3];
        for (unsigned row = 0; row < 3; ++row) {
            right[row] = down((int64_t)camera->rotation[row][0] * turn.rotation[0][0] +
                              (int64_t)camera->rotation[row][2] * turn.rotation[2][0], 4096);
        }
        unsigned depth_sum = 0;
        for (unsigned i = 0; i < 4; ++i) {
            int x = left + (int)(i & 1) * width, y = top + (int)(i >> 1) * height;
            int view[3];
            for (unsigned j = 0; j < 3; ++j)
                view[j] = center[j] + down((int64_t)x * right[j] + (int64_t)y * camera->rotation[j][1], 4096);
            if (!screen_point(view, camera->projection, &out->vertices[i].x, &out->vertices[i].y)) return 0;
            depth_sum += (unsigned)view[2];
        }
        out->bucket = depth_sum / 16 + 1;
    }
    if (out->bucket >= 4096) return 0;
    unsigned tx = 1008u + u16le(descriptor + 4), ty = 256u + u16le(descriptor + 6);
    unsigned u = 4 * (tx % 64), v = ty % 256;
    out->material = (BfmForestRasterMaterial){
        .page = (uint16_t)(tx / 64 | (ty / 256) << 4 | 1u << 5),
        .clut = (413u << 6) | (352u / 16), .count = 4, .textured = 1
    };
    out->slot = 0;
    for (unsigned i = 0; i < 4; ++i) {
        out->vertices[i].u = (uint8_t)(u + (i & 1) * (width - 1));
        out->vertices[i].v = (uint8_t)(v + (i >> 1) * (height - 1));
        out->vertices[i].rgb = 0x808080;
    }
    return 1;
}
