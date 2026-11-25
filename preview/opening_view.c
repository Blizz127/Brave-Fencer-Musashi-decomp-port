/* SPDX-License-Identifier: MIT */
#include "scene_views.h"
#include "scene_tiles.h"
#include <stdio.h>
#include <stdlib.h>

enum { WIDTH = 320, HEIGHT = 240, LAST_FIELD = 696 };
enum { TITLE_WIDTH = 256, TITLE_HEIGHT = 64 };
enum { CREDIT_X = 80, CREDIT_Y = 104, CREDIT_WIDTH = 160, CREDIT_HEIGHT = 32 };

typedef struct {
    uint16_t sky[480][320];
    uint16_t title_ink[TITLE_HEIGHT][TITLE_WIDTH];
    uint16_t title_mask[TITLE_HEIGHT][TITLE_WIDTH];
    uint16_t lettering[CREDIT_HEIGHT][CREDIT_WIDTH];
    uint16_t silhouette[CREDIT_HEIGHT][CREDIT_WIDTH];
} OpeningView;

/* The opening paints the sky with screen-aligned textured quads. Nearest
 * texel interpolation at a half-texel lands on opposite sides of the tie
 * in their two triangles when the texture step is fractional in 16.16. */
static unsigned sample_axis(unsigned coordinate, unsigned extent,
                            unsigned last_texel, int first_triangle)
{
    return (2 * coordinate * last_texel + extent - (first_triangle && (last_texel * 65536u) % extent ? 1u : 0u))
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
    /* After the game title fades, the camera tilts down by three world
     * pixels every two 30 Hz animation ticks. */
    unsigned pan_ticks = fields > 508 ? (fields - 508) / 2 : 0;
    int pan = (int)((3 * pan_ticks + 1) / 2);
    if (pan > 239) pan = 239;
    return (SkyProjection){
        160 - 160 * scale / 4096, 160 + 160 * scale / 4096,
        160 + 96 * scale / 4096, 120 - 120 * scale / 4096 - pan,
        120 + 136 * scale / 4096 - pan, 120 + 360 * scale / 4096 - pan
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
    for (unsigned y = 0; y < TITLE_HEIGHT; ++y) {
        for (unsigned x = 0; x < TITLE_WIDTH; ++x) {
            unsigned shift = 4 * (x % 4);
            unsigned page_x = 896 + x / 4;
            unsigned ink = (words[320 + y][page_x] >> shift) & 15;
            unsigned mask = (words[400 + y][page_x] >> shift) & 15;
            view->title_ink[y][x] = words[258][368 + ink];
            view->title_mask[y][x] = words[260][368 + mask];
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

/* Each line of the game title opens from its middle. The top and bottom
 * strips grow by one source row per animation tick and then fade together. */
static uint16_t title_line(const OpeningView *view, uint16_t back, int x, int y,
                           int center, int first_row, int rows, unsigned level)
{
    int dy = y - center;
    if (dy < -rows || dy > rows)
        return back;
    int row = first_row + (dy < 0 ? dy + rows : 31 - rows + dy);
    return blend_credit(back, view->title_mask[row][x],
                        view->title_ink[row][x], level);
}

static uint16_t game_title(const OpeningView *view, uint16_t back, unsigned fields,
                           int x, int y)
{
    if (fields >= 324 && fields < 506 && x >= 32 && x < 288 &&
        y >= 88 && y <= 152) {
        int rows = (int)((fields - 324) / 2);
        if (rows > 16) rows = 16;
        unsigned level = 8 * ((506 - (fields & ~1u)) / 2);
        if (level > 128) level = 128;
        back = title_line(view, back, x - 32, y, 136, 32, rows, level);
        back = title_line(view, back, x - 32, y, 104, 0, rows, level);
    }
    if (fields >= 294 && fields < 356 && (y == 104 || y == 136)) {
        int tick = 1 + (int)((fields - 294) / 2);
        int length = tick * (26 + 6 * tick);
        if (length > 256) length = 256;
        int left = y == 104 ? 32 : 288 - length;
        if (x >= left && x <= left + length) {
            int start = 16 * (tick + (tick >= 6));
            if (start > 255) start = 255;
            int color = start + (x - left) * (255 - start) / length;
            back = (uint16_t)((color >> 3) << (y == 104 ? 0 : 10));
        }
    }
    return back;
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
            if (fields >= 294 && fields < 506)
                word = game_title(view, word, fields, (int)x, (int)y);
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
