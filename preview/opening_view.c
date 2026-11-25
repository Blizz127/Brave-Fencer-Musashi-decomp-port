/* SPDX-License-Identifier: MIT */
#include "scene_views.h"
#include "scene_tiles.h"
#include <stdio.h>
#include <stdlib.h>

enum { WIDTH = 320, HEIGHT = 240, LAST_FIELD = 114 };

typedef struct {
    uint16_t moon[HEIGHT][WIDTH];
} OpeningView;

/* The opening paints the sky with screen-aligned textured quads. Nearest
 * texel interpolation at a half-texel lands on opposite sides of the tie
 * in their two triangles. Only the top pair is visible in the first shot. */
static unsigned sample_axis(unsigned coordinate, unsigned extent,
                            unsigned last_texel, int first_triangle)
{
    return (2 * coordinate * last_texel + extent - (first_triangle ? 1u : 0u))
           / (2 * extent);
}

static uint16_t sky_texel(const uint16_t words[512][1024], unsigned x, unsigned y)
{
    unsigned local_x, extent, last_u, u0, page_x, page_y;
    unsigned local_y = y + 52;
    if (x < 297) {
        local_x = x + 69;
        extent = 366;
        last_u = 255;
        u0 = 0;
        page_x = 896;
        page_y = 0;
    } else {
        local_x = x - 297;
        extent = 92;
        last_u = 63;
        u0 = 64;
        page_x = 960;
        page_y = 256;
    }
    int first_triangle = local_x * 367 + local_y * extent < extent * 367;
    unsigned u = u0 + sample_axis(local_x, extent, last_u, first_triangle);
    unsigned v = sample_axis(local_y, 367, 255, first_triangle);
    uint16_t pair = words[page_y + v][page_x + u / 2];
    unsigned index = (pair >> (8 * (u & 1))) & 255u;
    return words[503][index] & 0x7fffu;
}

int bfm_opening_load(const MusashiDiscMedia *disc, void **scene,
                     char *error, size_t error_size)
{
    if (!scene)
        return 0;
    *scene = NULL;
    uint16_t (*words)[1024] = calloc(512, sizeof(*words));
    OpeningView *view = calloc(1, sizeof(*view));
    if (!words || !view || !bfm_scene_read_tiles(disc, "SC01.CD", 0, words)) {
        free(words);
        free(view);
        if (error && error_size)
            snprintf(error, error_size, "Could not load the opening artwork from SC01.CD.");
        return 0;
    }
    for (unsigned y = 10; y < HEIGHT - 10; ++y)
        for (unsigned x = 0; x < WIDTH; ++x)
            view->moon[y][x] = sky_texel(words, x, y);
    free(words);
    *scene = view;
    return 1;
}

static uint32_t faded_channel(unsigned value, unsigned subtraction)
{
    return value > subtraction ? (value - subtraction) * 255 / 31 : 0;
}

int bfm_opening_frame(void *scene, unsigned fields, uint32_t *pixels,
                      int width, int height)
{
    const OpeningView *view = scene;
    if (!view || !pixels || width != WIDTH || height != HEIGHT)
        return -1;
    if (fields > LAST_FIELD)
        return 0;

    /* The original subtracts the same shade on each RGB555 channel and
     * advances it every other display field, then holds the moon fully lit. */
    unsigned darkness = fields < 34 ? 33 - 2 * (fields / 2) : 0;
    for (unsigned y = 0; y < HEIGHT; ++y) {
        for (unsigned x = 0; x < WIDTH; ++x) {
            uint16_t word = view->moon[y][x];
            uint32_t r = faded_channel(word & 31, darkness);
            uint32_t g = faded_channel((word >> 5) & 31, darkness);
            uint32_t b = faded_channel((word >> 10) & 31, darkness);
            pixels[y * WIDTH + x] = UINT32_C(0xff000000) | (r << 16) | (g << 8) | b;
        }
    }
    return 1;
}

void bfm_opening_free(void *scene)
{
    free(scene);
}
