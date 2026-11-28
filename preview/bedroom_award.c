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

static void pickup_rays(const uint8_t geometry[32], const int center[3], int age,
                        unsigned slots, const BfmForestRaster *raster,
                        const BfmForestRasterMaterial *material)
{
    int fade = age > 9 ? age - 9 : 0;
    int height = fade ? 4096 : 48 * age * (age + 1);
    int width = fade ? 4096 - 128 * fade : height;
    unsigned turn = fade ? 9 * 113 + 34 * fade : 113 * age;
    /* Each surviving ray darkens the common palette immediately before
     * submitting its quad. Failed allocations leave its colors untouched. */
    int survivors = (slots & 1u) + ((slots >> 1) & 1u) + ((slots >> 2) & 1u);
    int dim = fade ? 4 * survivors * (fade - 1) : 0;
    for (unsigned ray = 0; ray < 3; ++ray) {
        if (!(slots & (1u << ray))) continue;
        if (fade) dim += 4;
        if (dim >= 192) break;
        unsigned angle = turn + ray * 1365;
        int s = sine(angle), c = sine(angle + 1024);
        /* The figure scales each rotation column before transforming the
         * mounted corners. These two fixed-point rounding steps are visible. */
        int xx = (c * width) >> 12, xy = (-s * height) >> 12;
        int yx = (s * width) >> 12, yy = (c * height) >> 12;
        BfmForestRasterVertex quad[4];
        for (unsigned k = 0; k < 4; ++k) {
            int x = component(geometry + k * 8), y = component(geometry + k * 8 + 2);
            quad[k] = (BfmForestRasterVertex){
                .x = center[0] + ((xx * x + xy * y) >> 12),
                .y = center[1] + ((yx * x + yy * y) >> 12),
                .rgb = gold((k < 2 ? 32 : 192) - dim)
            };
        }
        bfm_forest_raster_face(raster, material, quad);
    }
}

static void pickup_ring(const BfmBedroomView *view, const int center[3], int age,
                        const BfmForestRaster *raster, const BfmForestRasterMaterial *material)
{
    /* The ring already stepped its palette once at zero scale. Its projected
     * outer rim supplies the integer coordinates for the inset. */
    int dim = 10 * (age + 1);
    if (dim >= 192 || center[2] / 4 + 1 >= 4096) return;
    int scale = 8 * age * (age + 1);
    int reciprocal = (int)(((int64_t)view->focal * 65536 + center[2] / 2) / center[2]);
    BfmForestRasterVertex outer[12], inner[12];
    for (int i = 0; i < 12; ++i) {
        int step = i <= 6 ? i : 12 - i;
        if (step > 3) step = 6 - step;
        int x = (128 * sine(step * 341)) >> 12;
        int y = (128 * sine(step * 341 + 1024)) >> 12;
        if (i > 6) x = -x;
        if (i > 3 && i < 9) y = -y;
        x = (int)(((int64_t)((x * scale) >> 12) * reciprocal) >> 16);
        y = (int)(((int64_t)((y * scale) >> 12) * reciprocal) >> 16);
        outer[i] = (BfmForestRasterVertex){
            .x=center[0]+x, .y=center[1]+y, .rgb=gold(192-dim)
        };
        inner[i] = (BfmForestRasterVertex){
            .x=center[0]+((3276*x) >> 12), .y=center[1]+((3276*y) >> 12), .rgb=gold(32-dim)
        };
    }
    for (int i = 0; i < 12; ++i) {
        int next = (i + 1) % 12;
        BfmForestRasterVertex quad[4] = {outer[i], outer[next], inner[i], inner[next]};
        bfm_forest_raster_face(raster, material, quad);
    }
}

void bfm_bedroom_award_draw_slots(const uint8_t geometry[32], const BfmBedroomView *view,
                                  const int16_t position[3], unsigned age, unsigned visible_slots,
                                  uint32_t pixels[240][320])
{
    if (!geometry || !view || !position || !pixels || !(visible_slots & 15) || !age || age > 40) return;
    int16_t world[3] = {position[0], (int16_t)(position[1] - 64), position[2]};
    int center[3];
    if (!bfm_bedroom_project(view, world, center)) return;
    BfmForestRaster raster = {.pixels=pixels, .top=10, .bottom=229};
    BfmForestRasterMaterial additive = {.page=32, .count=4, .translucent=1};
    pickup_rays(geometry, center, (int)age, visible_slots, &raster, &additive);
    if ((visible_slots & 8) && bfm_bedroom_project_packet(view, world, center))
        pickup_ring(view, center, (int)age, &raster, &additive);
}
