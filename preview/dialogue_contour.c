/* SPDX-License-Identifier: MIT */
#include "dialogue_contour.h"
#include <stdlib.h>

static void put(uint32_t pixels[240][320], int x, int y, uint32_t color, int top)
{
    if ((unsigned)x < 320 && y >= top && y < 240)
        pixels[y][x] = color;
}

static void edge_line(uint32_t pixels[240][320], BfmDialoguePoint a, BfmDialoguePoint b, uint32_t color, int top)
{
    if (a.x > b.x) { BfmDialoguePoint tmp = a; a = b; b = tmp; }
    int dx = abs(b.x - a.x), dy = -abs(b.y - a.y);
    int sx = a.x < b.x ? 1 : -1, sy = a.y < b.y ? 1 : -1;
    int err = dx + dy;
    if (dx > -dy) err -= (dx - dy - 1) / 2 - (dx - 1) / 2;
    for (;;) {
        put(pixels, a.x, a.y, color, top);
        if (a.x == b.x && a.y == b.y) break;
        int twice = 2 * err;
        if (twice > dy) { err += dy; a.x += sx; }
        if (twice < dx) { err += dx; a.y += sy; }
    }
}

void bfm_dialogue_contour(const BfmDialoguePoint border[BFM_DIALOGUE_CORNERS],
                         const BfmDialoguePoint inset[BFM_DIALOGUE_CORNERS],
                         uint32_t paper, uint32_t highlight, uint32_t outline,
                         int top, uint32_t pixels[240][320])
{
    int low = 240, high = 0;
    for (unsigned i = 0; i < BFM_DIALOGUE_CORNERS; ++i) {
        if (low > border[i].y) low = border[i].y;
        if (high < border[i].y) high = border[i].y;
    }
    if (low < top) low = top;
    if (high > 240) high = 240;
    for (int y = low; y < high; ++y) {
        int crossings[BFM_DIALOGUE_CORNERS], count = 0;
        for (unsigned i = 0; i < BFM_DIALOGUE_CORNERS; ++i) {
            BfmDialoguePoint a = border[i], b = border[(i + 1) % BFM_DIALOGUE_CORNERS];
            if ((y < a.y) == (y < b.y)) continue;
            if (a.y > b.y) { BfmDialoguePoint tmp = a; a = b; b = tmp; }
            int dy = b.y - a.y, n = (y - a.y) * (b.x - a.x);
            /* Cover integer samples from the left edge up to the right edge. */
            int x = a.x + n / dy + (n % dy > 0);
            int j = count++;
            while (j && crossings[j-1] > x) {
                crossings[j] = crossings[j-1]; --j;
            }
            crossings[j] = x;
        }
        for (int i = 1; i < count; i += 2)
            for (int x = crossings[i-1]; x < crossings[i]; ++x)
                put(pixels, x, y, paper, top);
    }
    for (unsigned i = 0; i < BFM_DIALOGUE_CORNERS; ++i)
        edge_line(pixels, inset[i], inset[(i + 1) % BFM_DIALOGUE_CORNERS], highlight, top);
    for (unsigned i = 0; i < BFM_DIALOGUE_CORNERS; ++i)
        edge_line(pixels, border[i], border[(i + 1) % BFM_DIALOGUE_CORNERS], outline, top);
}

/* A reflected textured quad is split along its diagonal. The span to the
 * right of that edge starts at the interpolated edge texel. The opposite
 * span starts at the left vertex, which can round a half texel differently. */
BfmDialoguePoint bfm_dialogue_reflected_texel(unsigned x, unsigned y, unsigned width, unsigned height,
                             unsigned last_v)
{
    const int one = 65536;
    int extent = (int)width - 1;
    int du = extent * one / (int)width;
    int diagonal = (int)((int64_t)y * width * one / height);
    int distance = (int)x * one - diagonal;
    int u = extent * one;
    if (distance < 0) {
        u -= (int)x * du;
    } else {
        u -= (int)y * (extent * one / (int)height);
        u -= (int)(((int64_t)distance * du + one - 1) / one);
    }
    unsigned v = y * (last_v * one / height);
    return (BfmDialoguePoint){(u + one / 2) / one, (int)(v + one / 2) / one};
}
