/* SPDX-License-Identifier: MIT */
#include "scene_views.h"
#include "scene_tiles.h"
#include <stdio.h>
#include <stdlib.h>

enum { WIDTH = 320, HEIGHT = 240, LAST_FIELD = 294 };
enum { CREDIT_X = 80, CREDIT_Y = 104, CREDIT_WIDTH = 160, CREDIT_HEIGHT = 32 };

typedef struct {
    uint16_t sky[480][320];
    uint16_t lettering[CREDIT_HEIGHT][CREDIT_WIDTH];
    uint16_t silhouette[CREDIT_HEIGHT][CREDIT_WIDTH];
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

static uint16_t read_sky(const uint16_t words[512][1024], unsigned x, unsigned y)
{
    unsigned page_x = x < 256 ? (y < 256 ? 896 : 768) : 960;
    unsigned page_y = x < 256 ? 0 : 256;
    unsigned u = x < 256 ? x : x - 256 + (y < 256 ? 64 : 0);
    unsigned v = y < 256 ? y : y - 256;
    uint16_t pair = words[page_y + v][page_x + u / 2];
    return words[503][(pair >> (8 * (u & 1))) & 255u] & 0x7fffu;
}

typedef struct {
    int left, right, seam, top, middle, bottom;
} SkyProjection;

static SkyProjection projection(unsigned fields)
{
    unsigned steps = fields > 248 ? (fields - 248) / 2 : 0;
    int scale = steps < 23 ? 5880 - 80 * (int)steps : 4096;
    return (SkyProjection){
        160 - 160 * scale / 4096, 160 + 160 * scale / 4096,
        160 + 96 * scale / 4096, 120 - 120 * scale / 4096,
        120 + 136 * scale / 4096, 120 + 360 * scale / 4096
    };
}

static uint16_t sky_texel(const OpeningView *view, const SkyProjection *p, int x, int y)
{
    int left = x < p->seam;
    int top = y < p->middle;
    unsigned local_x = (unsigned)(x - (left ? p->left : p->seam));
    unsigned local_y = (unsigned)(y - (top ? p->top : p->middle));
    unsigned w = (unsigned)(left ? p->seam - p->left : p->right - p->seam);
    unsigned h = (unsigned)(top ? p->middle - p->top : p->bottom - p->middle);
    int first_triangle = local_x * h + local_y * w < w * h;
    unsigned u = (left ? 0u : 256u) +
                 sample_axis(local_x, w, left ? 255u : 63u, first_triangle);
    unsigned v = (top ? 0u : 256u) +
                 sample_axis(local_y, h, top ? 255u : 223u, first_triangle);
    return view->sky[v][u];
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
    for (unsigned y = 0; y < 480; ++y)
        for (unsigned x = 0; x < WIDTH; ++x)
            view->sky[y][x] = read_sky(words, x, y);
    for (unsigned y = 0; y < CREDIT_HEIGHT; ++y) {
        for (unsigned x = 0; x < CREDIT_WIDTH; ++x) {
            unsigned shift = 4 * (x % 4);
            unsigned page_x = 896 + x / 4;
            unsigned letter = (words[256 + y][page_x] >> shift) & 15;
            unsigned mask = (words[288 + y][page_x] >> shift) & 15;
            view->lettering[y][x] = words[256][368 + letter];
            view->silhouette[y][x] = words[257][368 + mask];
        }
    }
    free(words);
    *scene = view;
    return 1;
}

static uint32_t faded_channel(unsigned value, unsigned subtraction)
{
    return value > subtraction ? (value - subtraction) * 255 / 31 : 0;
}

static unsigned credit_strength(unsigned fields)
{
    if (fields < 124 || fields >= 246)
        return 0;
    unsigned rise = (fields - 124) / 2;
    unsigned fall = (246 - (fields & ~1u)) / 2;
    unsigned steps = rise < fall ? rise : fall;
    return (steps < 16 ? steps : 16) * 8;
}

static uint16_t blend_credit(uint16_t backdrop, uint16_t silhouette, uint16_t letter,
                             unsigned strength)
{
    uint16_t result = 0;
    for (unsigned shift = 0; shift <= 10; shift += 5) {
        unsigned back = (backdrop >> shift) & 31;
        unsigned loss = ((silhouette >> shift) & 31) * strength / 128;
        unsigned ink = ((letter >> shift) & 31) * strength / 128;
        unsigned value = (back > loss ? back - loss : 0) + ink;
        result |= (uint16_t)((value < 32 ? value : 31) << shift);
    }
    return result;
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
    SkyProjection camera = projection(fields);
    unsigned strength = credit_strength(fields);
    for (unsigned y = 0; y < HEIGHT; ++y) {
        for (unsigned x = 0; x < WIDTH; ++x) {
            uint16_t word = y >= 10 && y < HEIGHT - 10
                                ? sky_texel(view, &camera, (int)x, (int)y) : 0;
            if (strength && y >= CREDIT_Y && y < CREDIT_Y + CREDIT_HEIGHT &&
                x >= CREDIT_X && x < CREDIT_X + CREDIT_WIDTH)
                word = blend_credit(word, view->silhouette[y - CREDIT_Y][x - CREDIT_X],
                                    view->lettering[y - CREDIT_Y][x - CREDIT_X], strength);
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
