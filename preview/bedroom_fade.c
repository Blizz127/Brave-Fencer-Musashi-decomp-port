/* SPDX-License-Identifier: MIT */
#include "bedroom_fade.h"

void bfm_bedroom_fade_begin(BfmBedroomFade *fade)
{
    *fade = (BfmBedroomFade){.remaining = 96, .top = 255, .bottom = 255, .visible = 1};
}

void bfm_bedroom_fade_step(BfmBedroomFade *fade)
{
    fade->visible = fade->remaining != 0;
    if (!fade->visible) return;
    if (--fade->remaining < 64) {
        unsigned *edge = fade->bottom ? &fade->bottom : &fade->top;
        *edge = *edge > 8 ? *edge - 8 : 0;
    }
}

static unsigned subtract_channel(unsigned channel, int shade, int phase)
{
    /* The software scanout used for the captured US route quantizes the
     * shaded difference at the absolute VRAM dither phase. */
    int difference = (int)(channel & 248u) - shade + phase;
    return difference > 0 ? (unsigned)(difference / 8) * 255u / 31u : 0;
}

void bfm_bedroom_fade_draw(const BfmBedroomFade *fade, uint32_t pixels[240][320])
{
    static const int dither[4][4] = {
        {-4, 0, -3, 1}, {2, -2, 3, -1},
        {-3, 1, -4, 0}, {3, -1, 2, -2}
    };
    if (!fade->visible) return;
    /* The gradient spans the full 240-line quad; the room's drawing area
     * clips its first and last ten lines. Both VRAM pages share the phase. */
    int step = ((int)fade->bottom - (int)fade->top) * 65536 / 240;
    int level = (int)fade->top * 65536 + 32768 + 10 * step;
    for (unsigned y = 10; y < 230; ++y, level += step) {
        int shade = level / 65536;
        for (unsigned x = 0; x < 320; ++x) {
            uint32_t p = pixels[y][x];
            int phase = dither[y & 3u][x & 3u];
            unsigned r = subtract_channel(p >> 16, shade, phase);
            unsigned g = subtract_channel(p >> 8, shade, phase);
            unsigned b = subtract_channel(p, shade, phase);
            pixels[y][x] = 0xff000000u | r << 16 | g << 8 | b;
        }
    }
}
