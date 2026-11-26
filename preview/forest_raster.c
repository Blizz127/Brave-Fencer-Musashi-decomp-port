/* SPDX-License-Identifier: MIT */
#include "forest_raster.h"

#include <stdint.h>
#include <stdlib.h>

enum { WIDTH = 320, HEIGHT = 240, ONE = 4096 };

typedef BfmForestRasterVertex Vertex;

typedef struct {
    int64_t dx, dy, origin;
} Plane;

static int minimum(int a, int b) { return a < b ? a : b; }
static int maximum(int a, int b) { return a > b ? a : b; }

static int line_offset(int step, int delta, int divisor)
{
    int64_t q12 = (int64_t)step * delta * ONE / divisor + ONE / 2;
    return (int)(q12 / ONE - (q12 % ONE < 0));
}

void bfm_forest_raster_additive_line(const BfmForestRaster *surface, const BfmForestRasterVertex line[2])
{
    if (!surface || !surface->pixels || !line) return;
    int64_t span_x = (int64_t)line[1].x - line[0].x;
    int64_t span_y = (int64_t)line[1].y - line[0].y;
    if (span_x < -1023 || span_x > 1023 || span_y < -511 || span_y > 511) return;
    int dx = (int)span_x, dy = (int)span_y;
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
    int divisor = steps ? steps : 1;
    for (int i = 0; i <= steps; ++i) {
        int x = line[0].x + line_offset(i, dx, divisor);
        int y = line[0].y + line_offset(i, dy, divisor);
        if (x < 0 || x >= 320 || y < 0 || y >= 240 || y < surface->top || y > surface->bottom)
            continue;
        uint32_t pixel = surface->pixels[y][x], color = 0xff000000u;
        for (unsigned k = 0; k < 3; ++k) {
            int start = (line[0].rgb >> (8*k)) & 255;
            int end = (line[1].rgb >> (8*k)) & 255;
            unsigned ink = (unsigned)(start + (end-start)*i/divisor) >> 3;
            unsigned shift = 16-8*k;
            unsigned value = ((pixel >> (shift+3)) & 31u) + ink;
            if (value > 31) value = 31;
            color |= (value*8u + value/4u) << shift;
        }
        surface->pixels[y][x] = color;
    }
}

static int64_t edge(const Vertex *a, const Vertex *b, int x, int y)
{
    return ((int64_t)b->x - a->x) * ((int64_t)y - a->y) -
           ((int64_t)b->y - a->y) * ((int64_t)x - a->x);
}

/* With positive winding in screen coordinates the top and left edges own
 * their integer samples. The adjacent strip triangle owns the other edges. */
static int closed_edge(const Vertex *a, const Vertex *b)
{
    return a->y > b->y || (a->y == b->y && a->x < b->x);
}

/* Console color and direct-color texture ramps have signed Q12 slopes.
 * For the forest's indexed materials, retain the exact affine gradients.
 * The retained doorway samples favor the lower index on exact half texels. */
static Plane attribute(const Vertex *a, const Vertex *b, const Vertex *c,
                       int av, int bv, int cv, int64_t area, int indexed)
{
    int64_t bx = (int64_t)b->x - a->x, by = (int64_t)b->y - a->y;
    int64_t cx = (int64_t)c->x - a->x, cy = (int64_t)c->y - a->y;
    Plane p;
    int64_t dx = (bv - av)*cy - (cv - av)*by;
    int64_t dy = (cv - av)*bx - (bv - av)*cx;
    if (indexed) {
        p.dx = 2*dx;
        p.dy = 2*dy;
        p.origin = (2*av + 1)*area - 1;
    } else {
        p.dx = dx * ONE / area;
        p.dy = dy * ONE / area;
        p.origin = av*ONE + ONE/2;
    }
    p.origin -= p.dx*a->x + p.dy*a->y;
    return p;
}

static int sample(const Plane *p, int x, int y, int64_t unit)
{
    int64_t value = p->origin + p->dx*x + p->dy*y;
    return (int)(value / unit - (value % unit < 0));
}

static uint16_t texture(const BfmForestRaster *surface, const BfmForestRasterMaterial *m,
                        unsigned u, unsigned v)
{
    unsigned depth = (m->page >> 7) & 3;
    unsigned x = (m->page & 15) * 64;
    unsigned y = ((m->page & 16) ? 256 : 0) + (v & 255);
    u &= 255;
    if (depth == 2)
        return surface->tiles[y][(x + u) & 1023];
    unsigned bits = depth ? 8 : 4;
    unsigned per_word = 16 / bits;
    uint16_t packed = surface->tiles[y][(x + u / per_word) & 1023];
    unsigned index = (packed >> ((u % per_word)*bits)) & ((1u << bits) - 1);
    unsigned palette_x = (m->clut & 63) * 16;
    return surface->tiles[(m->clut >> 6) & 511][(palette_x + index) & 1023];
}

static unsigned blend(unsigned source, unsigned destination, unsigned mode)
{
    switch (mode) {
    case 0: return (source + destination) / 2;
    case 1: return (unsigned)minimum(31, (int)(source + destination));
    case 2: return destination > source ? destination - source : 0;
    default: return (unsigned)minimum(31, (int)(destination + source / 4));
    }
}

static void paint(const BfmForestRaster *surface, const BfmForestRasterMaterial *m,
                  const Plane lights[3], const Plane uv[2], int64_t uv_unit, int x, int y, int field_levels)
{
    static const int8_t dither[4][4] = {
        {-4, 0,-3, 1}, { 2,-2, 3,-1}, {-3, 1,-4, 0}, { 3,-1, 2,-2}
    };
    uint16_t ink = 0;
    if (m->textured) {
        ink = texture(surface, m, (unsigned)sample(&uv[0],x,y,uv_unit),
                                  (unsigned)sample(&uv[1],x,y,uv_unit));
        if (!ink) return;
    }
    int shift = m->dither ? dither[y & 3][x & 3] : 0;
    int semi = m->translucent && (!m->textured || (ink & 0x8000));
    uint32_t pixel = UINT32_C(0xff000000);
    uint32_t background = surface->pixels[y][x];
    for (unsigned k = 0; k < 3; ++k) {
        int light = sample(&lights[k],x,y,ONE);
        int color = m->textured ? ((ink >> (5*k)) & 31)*light / 16 : light;
        unsigned level = (unsigned)minimum(255,maximum(0,color+shift)) / 8;
        unsigned channel = 16-8*k;
        if (semi)
            level = blend(level, (background >> (channel+3)) & 31, (m->page >> 5) & 3);
        unsigned display = field_levels ? level * 255 / 31 : level * 8 + level / 4;
        pixel |= display << channel;
    }
    surface->pixels[y][x] = pixel;
}

static int precedes(const Vertex *a, const Vertex *b)
{
    return a->x < b->x || (a->x == b->x && a->y < b->y);
}

static void raster_triangle(const BfmForestRaster *surface, const BfmForestRasterMaterial *m,
                     const Vertex *a, const Vertex *b, const Vertex *c, int field_levels)
{
    /* A cyclic permutation puts both attributes and coverage in one basis. */
    if (precedes(b,a) && precedes(b,c)) {
        const Vertex *first = a;
        a = b; b = c; c = first;
    } else if (precedes(c,a)) {
        const Vertex *first = a;
        a = c; c = b; b = first;
    }
    int max_x = maximum(b->x, c->x);
    int min_y = minimum(a->y, minimum(b->y, c->y));
    int max_y = maximum(a->y, maximum(b->y, c->y));
    int64_t area = edge(a,b,c->x,c->y);
    if (!area) return;
    if (area < 0) {
        const Vertex *tmp = b;
        b = c; c = tmp; area = -area;
    }

    int left = maximum(0,a->x);
    int right = minimum(WIDTH-1,max_x);
    int top = maximum(maximum(0,surface->top),min_y);
    int bottom = minimum(minimum(HEIGHT-1,surface->bottom),max_y);
    if (left > right || top > bottom) return;

    Plane lights[3], uv[2];
    for (unsigned k = 0; k < 3; ++k) {
        unsigned bit = 8*k;
        lights[k] = attribute(a,b,c,(a->rgb >> bit) & 255,(b->rgb >> bit) & 255,
                              (c->rgb >> bit) & 255,area,0);
    }
    int indexed = m->textured && ((m->page >> 7) & 3) < 2;
    int64_t uv_unit = indexed ? 2*area : ONE;
    if (m->textured) {
        uv[0] = attribute(a,b,c,a->u,b->u,c->u,area,indexed);
        uv[1] = attribute(a,b,c,a->v,b->v,c->v,area,indexed);
    }

    int ab_excluded = !closed_edge(a,b), bc_excluded = !closed_edge(b,c);
    int ca_excluded = !closed_edge(c,a);
    for (int y = top; y <= bottom; ++y) {
        int64_t ab = edge(a,b,left,y) - ab_excluded;
        int64_t bc = edge(b,c,left,y) - bc_excluded;
        int64_t ca = edge(c,a,left,y) - ca_excluded;
        for (int x = left; x <= right; ++x) {
            if (ab >= 0 && bc >= 0 && ca >= 0)
                paint(surface,m,lights,uv,uv_unit,x,y,field_levels);
            ab += (int64_t)a->y - b->y;
            bc += (int64_t)b->y - c->y;
            ca += (int64_t)c->y - a->y;
        }
    }
}

static int with_offset(int coordinate, int offset)
{
    unsigned bits = ((unsigned)coordinate + (unsigned)offset) & 2047u;
    return bits < 1024 ? (int)bits : (int)bits - 2048;
}

static void triangle(const BfmForestRaster *surface, const BfmForestRasterMaterial *m,
                     const Vertex *a, const Vertex *b, const Vertex *c, int dx, int dy,
                     int viewport_y, int field_levels)
{
    int left = minimum(a->x, minimum(b->x, c->x));
    int right = maximum(a->x, maximum(b->x, c->x));
    int top = minimum(a->y, minimum(b->y, c->y));
    int bottom = maximum(a->y, maximum(b->y, c->y));
    /* Check each submitted triangle before applying the drawing offset. */
    if ((int64_t)right - left > 1023 || (int64_t)bottom - top > 511) return;
    if (dx || dy) {
        Vertex shifted[] = {*a, *b, *c};
        for (unsigned i = 0; i < 3; ++i) {
            shifted[i].x = with_offset(shifted[i].x, dx);
            shifted[i].y = with_offset(shifted[i].y, dy) - viewport_y;
        }
        raster_triangle(surface, m, &shifted[0], &shifted[1], &shifted[2], field_levels);
    } else {
        raster_triangle(surface, m, a, b, c, field_levels);
    }
}

static void raster_face(const BfmForestRaster *surface, const BfmForestRasterMaterial *m,
                        const BfmForestRasterVertex corners[4], int offset_x, int offset_y,
                        int viewport_y, int field_levels)
{
    if (!surface || !surface->pixels || !m || !corners ||
        (m->count != 3 && m->count != 4) || (m->textured &&
        (!surface->tiles || ((m->page >> 7) & 3) == 3))) return;
    triangle(surface, m, &corners[0], &corners[1], &corners[2], offset_x, offset_y, viewport_y, field_levels);
    if (m->count == 4)
        triangle(surface, m, &corners[2], &corners[1], &corners[3], offset_x, offset_y, viewport_y, field_levels);
}

void bfm_forest_raster_buffer_face(const BfmForestRaster *surface,
                                   const BfmForestRasterMaterial *m,
                                   const BfmForestRasterVertex corners[4], unsigned buffer)
{
    if (buffer > 1) return;
    int viewport_y = (int)buffer * 240;
    raster_face(surface, m, corners, 160, 120 + viewport_y, viewport_y, 0);
}

void bfm_forest_raster_offset_face(const BfmForestRaster *surface,
                                   const BfmForestRasterMaterial *m,
                                   const BfmForestRasterVertex corners[4],
                                   int offset_x, int offset_y)
{
    raster_face(surface, m, corners, offset_x, offset_y, 0, 0);
}

void bfm_forest_raster_face(const BfmForestRaster *surface,
                            const BfmForestRasterMaterial *m,
                            const BfmForestRasterVertex corners[4])
{
    raster_face(surface, m, corners, 0, 0, 0, 0);
}

void bfm_field_raster_face(const BfmForestRaster *surface, const BfmForestRasterMaterial *m,
                           const BfmForestRasterVertex corners[4])
{
    raster_face(surface, m, corners, 0, 0, 0, 1);
}
