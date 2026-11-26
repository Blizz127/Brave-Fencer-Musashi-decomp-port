/* SPDX-License-Identifier: MIT */
#include "bedroom_award.h"
#include "forest_raster.h"
#include <math.h>

static int sine(unsigned angle)
{
    return (int)lround(4096 * sin((angle & 4095u) * (6.2831853071795864769 / 4096)));
}

static int component(const uint8_t *p)
{
    unsigned v = p[0] | (unsigned)p[1] << 8;
    return v < 32768 ? (int)v : (int)v - 65536;
}

static uint32_t gold(int level)
{
    return level > 0 ? (unsigned)level * 257u : 0;
}

static BfmForestRasterVertex vertex(const int center[3], int x, int y, unsigned angle,
                                    int width, int height, int light)
{
    int s = sine(angle), c = sine(angle + 1024);
    x = (x * width) >> 12;
    y = (y * height) >> 12;
    return (BfmForestRasterVertex){
        .x = center[0] + ((x*c - y*s) >> 12),
        .y = center[1] + ((x*s + y*c) >> 12),
        .rgb = gold(light)
    };
}

void bfm_bedroom_award_draw(const uint8_t geometry[32], const BfmBedroomView *view,
                            const int16_t position[3], unsigned age, uint32_t pixels[240][320])
{
    if (!geometry || !view || !age || age > 25) return;
    int16_t world[3] = {(int16_t)position[0], (int16_t)(position[1]-64), (int16_t)position[2]};
    int center[3];
    if (!bfm_bedroom_project(view, world, center)) return;
    BfmForestRaster raster = {.pixels=pixels, .top=10, .bottom=229};
    BfmForestRasterMaterial additive = {.page=32, .count=4, .translucent=1};
    BfmForestRasterVertex v[4];
    int t = (int)age, fade = t > 9 ? t-9 : 0;
    int width = fade ? 4096 - 128*fade : 48*t*(t+1), height = fade ? 4096 : width;
    unsigned rotation = fade ? 9*113 + 34u*(unsigned)fade : age*113;
    for (unsigned ray = 0; ray < 3; ++ray) {
        int dim = fade ? 12*(fade-1) + 4*((int)ray+1) : 0;
        for (unsigned k = 0; k < 4; ++k) {
            const uint8_t *p = geometry + k*8;
            v[k] = vertex(center, component(p), component(p+2), rotation+ray*1365,
                          width, height, (k < 2 ? 32 : 192) - dim);
        }
        bfm_forest_raster_face(&raster, &additive, v);
    }
    int outer = 192 - 10*t;
    if (outer <= 0) return;
    int scale = 8*t*(t+1);
    for (unsigned edge = 0; edge < 12; ++edge) {
        for (unsigned end = 0; end < 2; ++end) {
            unsigned angle = ((edge+end)*4096/12) & 4095u;
            v[end*2] = vertex(center, 0, 128, angle, scale, scale, outer);
            v[end*2+1] = vertex(center, 0, 128*3276/4096, angle, scale, scale, 32-10*t);
        }
        bfm_forest_raster_face(&raster, &additive, v);
    }
}
