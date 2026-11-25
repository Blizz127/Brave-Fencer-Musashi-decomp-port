/* SPDX-License-Identifier: MIT */
#include "stair_actors.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { ACTORS = 3, PARTS = 26, MAX_VERTS = 128, MAX_FACES = 128,
       MAX_DRAW = 2048, ROOT_POSES = 601, W = 320, H = 240 };
typedef struct { int v[3]; } Vec;
typedef struct { int a[3][3]; } Mat;
typedef struct {
    uint16_t vertex[4], normal[4], palette, page;
    uint8_t uv[4][2], rgb[4][3], count, textured, lit, blend;
} Face;
typedef struct {
    const uint8_t *vertices, *normals;
    unsigned nv, nn, nf;
    Face faces[MAX_FACES];
} Model;
typedef struct {
    const uint8_t *bones, *roots, *landing_bones;
    unsigned first, count, frames;
} Performer;
typedef struct {
    const Face *face;
    Vec point[4], shade[4];
    int depth;
    unsigned sequence;
} Draw;
typedef struct { Vec normal; int32_t offset; } Plane;
typedef struct { Plane side[5]; unsigned count; } FloorSurface;
struct BfmStairActors {
    FloorSurface floor[512];
    size_t floor_count;
    Vec spawn[3];
    Face shadow;
    int shadow_left, shadow_top;
    Model parts[PARTS];
    Performer actor[ACTORS];
    const uint8_t *princess_landing;
    Draw order[MAX_DRAW];
    size_t used, next;
};

static unsigned word(const uint8_t *p) { return p[0] | (unsigned)p[1] << 8; }
static int signed_word(const uint8_t *p)
{
    unsigned n = word(p);
    return n < 32768 ? (int)n : (int)n - 65536;
}
static uint32_t longword(const uint8_t *p)
{
    return word(p) | (uint32_t)word(p + 2) << 16;
}
static Vec triple(const uint8_t *p)
{
    return (Vec){{signed_word(p), signed_word(p + 2), signed_word(p + 4)}};
}
static int within(size_t bytes, size_t pos, size_t size)
{
    return pos <= bytes && size <= bytes - pos;
}
static int clamp(int x, int low, int high) { return x < low ? low : x > high ? high : x; }
static int floored(int64_t n, int d)
{
    return (int)(n < 0 ? -((-n + d - 1) / d) : n / d);
}
static int fixed(int64_t n) { return floored(n, 4096); }

static int load_face(Face *f, const uint8_t *p, unsigned words, unsigned flags,
                     unsigned mode, unsigned nv, unsigned nn)
{
    unsigned n = mode & 8 ? 4u : 3u;
    int textured = (mode & 4) != 0, lit = (flags & 1) == 0;
    unsigned colors = textured ? (lit ? 0u : 1u) : ((flags & 4 || (!lit && (mode & 16))) ? n : 1u);
    unsigned uv_words = textured ? n : 0u;
    if ((mode & 0xe0u) != 0x20 || (flags & ~5u) ||
        (lit && !(mode & 16)) || (mode & 1u) != (unsigned)!lit ||
        words != uv_words + colors + (lit ? n : (n + 1) / 2)) return 0;
    f->count = (uint8_t)n;
    f->textured = (uint8_t)textured;
    f->lit = (uint8_t)lit;
    f->blend = (uint8_t)((mode >> 1) & 1);
    if (textured) {
        f->palette = (uint16_t)word(p + 2);
        f->page = (uint16_t)word(p + 6);
        if ((f->page >> 7 & 3) > 1 || (f->palette >> 6) >= 512) return 0;
        unsigned colors_per_page = (f->page & 128) ? 256u : 16u;
        if ((f->palette & 63u) * 16 + colors_per_page > 1024) return 0;
        for (unsigned j = 0; j < n; ++j) memcpy(f->uv[j], p + 4 * j, 2);
        p += 4 * n;
    }
    for (unsigned j = 0; j < n; ++j) {
        if (colors) memcpy(f->rgb[j], p + (colors > 1 ? 4 * j : 0), 3);
        else memset(f->rgb[j], 128, 3);
    }
    p += colors * 4;
    for (unsigned j = 0; j < n; ++j) {
        unsigned normal = lit ? word(p + 4 * j) : 0;
        unsigned vertex = lit ? word(p + 4 * j + 2) : word(p + 2 * j);
        if (vertex >= nv || (lit && normal >= nn)) return 0;
        f->vertex[j] = (uint16_t)vertex;
        f->normal[j] = (uint16_t)normal;
    }
    return 1;
}

static int load_model(Model *m, const uint8_t *s, size_t bytes)
{
    if (bytes < 40 || longword(s) != 65 || longword(s + 4) || longword(s + 8) != 1)
        return 0;
    s += 12; bytes -= 12;
    unsigned vo = longword(s), nv = longword(s + 4), no = longword(s + 8), nn = longword(s + 12);
    unsigned po = longword(s + 16), nf = longword(s + 20);
    if (longword(s + 24) || !nv || nv > MAX_VERTS || nn > MAX_VERTS || !nf || nf > MAX_FACES ||
        po < 28 || po > vo || vo > no || !within(bytes, vo, 8 * nv) ||
        !within(bytes, no, 8 * nn) || 8 * nv != no - vo) return 0;
    m->nv = nv; m->nn = nn; m->nf = nf; m->vertices = s + vo; m->normals = s + no;
    for (unsigned i = 0; i < nf; ++i) {
        if (!within(vo, po, 4)) return 0;
        const uint8_t *p = s + po;
        unsigned words = p[1];
        po += 4;
        if (!within(vo, po, 4 * words) ||
            !load_face(m->faces + i, s + po, words, p[2], p[3], nv, nn)) return 0;
        po += 4 * words;
    }
    return 1;
}

/* Both the stairs and the flat landing use convex collision faces. One
 * plane supplies the ground height, the remaining planes bound its edges. */
static int read_floor(BfmStairActors *a, const uint8_t *scene, size_t bytes)
{
    const size_t at = 0x67f7c;
    if (!within(bytes, at, 32)) return 0;
    const uint8_t *grid = scene + at;
    size_t distance = longword(grid + 12), normal = longword(grid + 16);
    size_t indices = longword(grid + 20), faces = longword(grid + 24), quads = longword(grid + 28);
    if (distance < 32 || distance > normal || normal > indices || indices > faces ||
        faces > quads || quads > bytes - at || (normal-distance)%4 || (indices-normal)%8 ||
        (faces-indices)%2 || (quads-faces)%18) return 0;
    size_t triangles = (quads-faces)/18, quad_count = 0;
    for (size_t pos = indices; pos < faces; pos += 2) {
        unsigned id = word(grid + pos);
        if (id < 0x8000) {
            if (id >= triangles) return 0;
        } else {
            size_t used = 1u + (id & 0x7fffu);
            if (used > quad_count) quad_count = used;
        }
    }
    if (triangles > 512 || quad_count > 512-triangles ||
        quad_count > (bytes-at-quads)/22) return 0;
    a->floor_count = triangles + quad_count;
    for (size_t i = 0; i < a->floor_count; ++i) {
        unsigned planes = i < triangles ? 4u : 5u;
        const uint8_t *face = i < triangles ? grid + faces + 18*i : grid + quads + 22*(i-triangles);
        a->floor[i].count = planes;
        for (unsigned j = 0; j < planes; ++j) {
            unsigned n = word(face + 2 + 4*j), d = word(face + 4 + 4*j);
            if (n >= (indices-normal)/8 || d >= (normal-distance)/4) return 0;
            Plane *plane = &a->floor[i].side[j];
            plane->normal = triple(grid + normal + 8*n);
            uint32_t raw = longword(grid + distance + 4*d);
            plane->offset = raw > INT32_MAX ? (int32_t)((int64_t)raw - INT64_C(4294967296)) : (int32_t)raw;
        }
    }
    return a->floor_count != 0;
}

static int64_t plane_value(Plane p, Vec v)
{
    int64_t sum = p.offset;
    for (unsigned j = 0; j < 3; ++j) sum += (int64_t)p.normal.v[j] * v.v[j];
    return sum;
}

static int ground(const BfmStairActors *a, Vec position, Vec *normal, int *height)
{
    int nearest = INT32_MAX;
    for (size_t i = 0; i < a->floor_count; ++i) {
        const Plane *p = a->floor[i].side;
        if (p[0].normal.v[1] >= -1024) continue;
        Vec test = {{position.v[0], 0, position.v[2]}};
        test.v[1] = floored(plane_value(p[0], test), -p[0].normal.v[1]);
        if (test.v[1] < -32768 || test.v[1] > 32767 ||
            plane_value(p[1], test) < -4096 || plane_value(p[2], test) < -4096 ||
            plane_value(p[3], test) < -4096 ||
            (a->floor[i].count == 5 && plane_value(p[4], test) < -4096)) continue;
        int gap = abs(test.v[1] - position.v[1]);
        if (gap < nearest) {
            nearest = gap;
            *normal = p[0].normal;
            *height = test.v[1];
        }
    }
    return nearest != INT32_MAX;
}

static int read_effects(BfmStairActors *a, const uint8_t *s, size_t bytes)
{
    const size_t locations = 0x60ba0, card = 0x5d018;
    if (!within(bytes, locations, 24) || !within(bytes, card, 12) || !read_floor(a,s,bytes)) return 0;
    for (unsigned i = 0; i < 3; ++i) a->spawn[i] = triple(s + locations + 8*i);
    const uint8_t *desc = s + card;
    unsigned w = desc[2], h = desc[3];
    if (!w || !h || w > 127 || h > 95) return 0;
    a->shadow_left = signed_word(desc + 8);
    a->shadow_top = signed_word(desc + 10);
    Face *f = &a->shadow;
    f->count = 4; f->textured = 1; f->blend = 1;
    f->page = 0x59; f->palette = 0x4516;
    for (unsigned j = 0; j < 4; ++j) {
        f->uv[j][0] = (uint8_t)(64 + ((j & 1) ? w-1 : 0));
        f->uv[j][1] = (uint8_t)(160 + ((j & 2) ? h-1 : 0));
    }
    return 1;
}

BfmStairActors *bfm_stair_actors_load(const uint8_t *s, size_t bytes)
{
    /* The US stairwell has one loose rock and three entering models.
     * Pointer lists refer to the member's original load address. Pose frames
     * store a local translation and three angles per piece, independent of
     * the scene track which moves the whole figure. */
    static const struct { unsigned list, bones, root, count, frames, landing_bones, landing_frames; } layout[ACTORS] = {
        {0x73be0, 0x7cff4, 0xaf764, 8, 20, 0x7d774, 14},
        {0x7634c, 0x823c4, 0xb3d4c, 10, 12, 0x82964, 23},
        {0x7848c, 0x86b7c, 0xbe8e4, 7, 12, 0x86834, 10}
    };
    const uint32_t load_address = UINT32_C(0x80128158);
    const unsigned rock = 0x71604u;
    if (!s || !within(bytes, rock, 40)) return NULL;
    BfmStairActors *out = calloc(1, sizeof(*out));
    if (!out || !read_effects(out, s, bytes) || !load_model(&out->parts[0], s + rock, bytes - rock)) goto fail;
    if (!within(bytes, 0xb0a30, 100*8)) goto fail;
    out->princess_landing = s + 0xb0a30;
    unsigned part = 1;
    unsigned total_faces = 3 * out->parts[0].nf + ACTORS;
    for (unsigned i = 0; i < ACTORS; ++i) {
        unsigned n = layout[i].count, frames = layout[i].frames;
        if (!within(bytes, layout[i].list, 4 * (n + 1)) ||
            !within(bytes, layout[i].bones, 12 * n * frames) ||
            !within(bytes, layout[i].landing_bones, 12 * n * layout[i].landing_frames) ||
            !within(bytes, layout[i].root, ROOT_POSES * 8)) goto fail;
        const uint8_t *list = s + layout[i].list;
        if (longword(list + 4 * n)) goto fail;
        out->actor[i] = (Performer){s + layout[i].bones, s + layout[i].root, s + layout[i].landing_bones, part, n, frames};
        for (unsigned j = 0; j < n; ++j, ++part) {
            uint32_t pointer = longword(list + 4 * j);
            if (pointer < load_address) goto fail;
            size_t pos = pointer - load_address;
            if (!within(bytes, pos, 40) || !load_model(out->parts + part, s + pos, bytes - pos))
                goto fail;
            total_faces += out->parts[part].nf;
            if (total_faces > MAX_DRAW) goto fail;
        }
    }
    return out;
fail:
    free(out);
    return NULL;
}

void bfm_stair_actors_free(BfmStairActors *actors) { free(actors); }

static Vec transform(Mat m, Vec p)
{
    Vec q;
    for (unsigned i = 0; i < 3; ++i) {
        int64_t sum = 0;
        for (unsigned k = 0; k < 3; ++k) sum += (int64_t)m.a[i][k] * p.v[k];
        q.v[i] = fixed(sum);
    }
    return q;
}
static Vec add(Vec p, Vec q)
{
    for (unsigned i = 0; i < 3; ++i) p.v[i] += q.v[i];
    return p;
}
static Mat multiply(Mat a, Mat b)
{
    Mat r;
    for (unsigned j = 0; j < 3; ++j) {
        Vec v = {{b.a[0][j], b.a[1][j], b.a[2][j]}};
        v = transform(a, v);
        for (unsigned i = 0; i < 3; ++i) r.a[i][j] = v.v[i];
    }
    return r;
}
static Mat rotation(unsigned axis, int units)
{
    double rad = units * (6.2831853071795864769 / 4096.0);
    int c = (int)lround(cos(rad) * 4096), s = (int)lround(sin(rad) * 4096);
    Mat r = {{{4096,0,0},{0,4096,0},{0,0,4096}}};
    unsigned a = (axis + 1) % 3, b = (axis + 2) % 3;
    r.a[a][a] = c; r.a[b][b] = c; r.a[a][b] = -s; r.a[b][a] = s;
    return r;
}
static Mat angles(Vec a)
{
    return multiply(multiply(rotation(2, a.v[2]), rotation(1, a.v[1])), rotation(0, a.v[0]));
}
static int edge(Vec a, Vec b, int x, int y)
{
    return (b.v[0] - a.v[0]) * (y - a.v[1]) - (b.v[1] - a.v[1]) * (x - a.v[0]);
}
static Vec project(Vec p)
{
    int z = p.v[2], scale = z > 0 ? clamp((256 * 65536 + z / 2) / z, 0, 131071) : 131071;
    p.v[0] = 160 + clamp(floored((int64_t)p.v[0] * scale, 65536), -1024, 1023);
    p.v[1] = 120 + clamp(floored((int64_t)p.v[1] * scale, 65536), -1024, 1023);
    return p;
}
static const Mat stage_directions = {{{0, -4267, 0}, {0, -1861, 3723}, {0, 4096, 0}}};
static const Mat stage_colors = {{{2457, 1317, 1236}, {819, 1558, 819}, {417, 1317, 1638}}};

static void prepare_model(BfmStairActors *group, const Model *m, Mat model, Vec origin, Mat local)
{
    Mat lighting = multiply(stage_directions, local);
    Vec pts[MAX_VERTS], intensity[MAX_VERTS];
    for (unsigned i = 0; i < m->nv; ++i)
        pts[i] = project(add(origin, transform(model, triple(m->vertices + 8 * i))));
    for (unsigned i = 0; i < m->nn; ++i) {
        Vec energy = transform(lighting, triple(m->normals + 8 * i));
        for (unsigned j = 0; j < 3; ++j) energy.v[j] = clamp(energy.v[j], 0, 32767);
        intensity[i] = add((Vec){{2048,2048,2048}}, transform(stage_colors, energy));
    }
    for (unsigned i = 0; i < m->nf && group->used < MAX_DRAW; ++i) {
        const Face *f = m->faces + i;
        Draw *d = group->order + group->used;
        int depth = 0, near = 65535;
        for (unsigned j = 0; j < f->count; ++j) {
            d->point[j] = pts[f->vertex[j]];
            int z = d->point[j].v[2];
            depth += z;
            if (z < near) near = z;
        }
        if (near <= 0 || edge(d->point[0], d->point[1], d->point[2].v[0], d->point[2].v[1]) <= 0)
            continue;
        for (unsigned j = 0; j < f->count; ++j)
            for (unsigned k = 0; k < 3; ++k) {
                int rgb = f->rgb[j][k];
                if (f->lit) rgb = rgb * intensity[f->normal[j]].v[k] / 4096;
                d->shade[j].v[k] = clamp(rgb, 0, 255);
            }
        d->sequence = (unsigned)group->used;
        d->depth = depth / (4 * f->count);
        d->face = f;
        ++group->used;
    }
}

static int farthest(const void *one, const void *two)
{
    const Draw *a = one, *b = two;
    if (a->depth != b->depth) return a->depth > b->depth ? -1 : 1;
    return a->sequence > b->sequence ? -1 : a->sequence < b->sequence;
}

static uint16_t texture(const Face *face, const uint16_t tiles[512][1024], unsigned u, unsigned v)
{
    unsigned bits = (face->page & 128) ? 8u : 4u, count = 16 / bits;
    unsigned row = (face->page & 16) * 16 + v;
    unsigned col = ((face->page & 15) * 64 + u / count) & 1023u;
    unsigned idx = (tiles[row][col] >> (bits * (u % count))) & ((1u << bits) - 1u);
    return tiles[face->palette >> 6][(face->palette & 63) * 16 + idx];
}

static int closed(Vec a, Vec b)
{
    return a.v[1] > b.v[1] || (a.v[1] == b.v[1] && a.v[0] < b.v[0]);
}
static void draw_triangle(const Draw *d, const uint16_t tiles[512][1024], uint32_t *out,
                          unsigned i, unsigned j, unsigned k)
{
    const Face *f = d->face;
    Vec a = d->point[i], b = d->point[j], c = d->point[k];
    int area = edge(a, b, c.v[0], c.v[1]);
    if (area < 0) {
        Vec tmp = a; a = b; b = tmp;
        unsigned index = i; i = j; j = index;
        area = -area;
    }
    if (!area) return;
    int x0 = clamp(a.v[0], 0, W - 1), x1 = x0;
    int y0 = clamp(a.v[1], 10, H - 11), y1 = y0;
    for (unsigned n = 0; n < 2; ++n) {
        Vec p = n ? c : b;
        if (p.v[0] < x0) x0 = clamp(p.v[0], 0, W - 1);
        if (p.v[0] > x1) x1 = clamp(p.v[0], 0, W - 1);
        if (p.v[1] < y0) y0 = clamp(p.v[1], 10, H - 11);
        if (p.v[1] > y1) y1 = clamp(p.v[1], 10, H - 11);
    }
    int ca = closed(b, c), cb = closed(c, a), cc = closed(a, b);
    for (int y = y0; y <= y1; ++y) for (int x = x0; x <= x1; ++x) {
        int wa = edge(b, c, x, y), wb = edge(c, a, x, y), wc = area - wa - wb;
        if (wa < !ca || wb < !cb || wc < !cc) continue;
        uint16_t ink = 0;
        if (f->textured) {
            unsigned coord[2];
            for (unsigned ch = 0; ch < 2; ++ch)
                coord[ch] = (unsigned)(((int64_t)wa * f->uv[i][ch] + (int64_t)wb * f->uv[j][ch] +
                                        (int64_t)wc * f->uv[k][ch] + area / 2) / area);
            ink = texture(f, tiles, coord[0], coord[1]);
            if (!ink) continue;
        }
        uint32_t pixel = UINT32_C(0xff000000);
        for (unsigned ch = 0; ch < 3; ++ch) {
            int shade = (int)(((int64_t)wa * d->shade[i].v[ch] + (int64_t)wb * d->shade[j].v[ch] +
                               (int64_t)wc * d->shade[k].v[ch] + area / 2) / area);
            if (f->textured) shade = (int)((ink >> (5 * ch)) & 31u) * shade / 16;
            int channel = clamp(shade, 0, 255) / 8;
            unsigned shift = 16 - 8 * ch;
            if (f->blend && (!f->textured || (ink & 0x8000))) {
                int old = (int)(((out[y * W + x] >> shift) & 255u) * 31 + 127) / 255;
                unsigned blend = f->textured ? ((f->page >> 5) & 3u) : 0;
                if (blend == 0) channel = (old + channel) / 2;
                else if (blend == 1) channel = clamp(old + channel, 0, 31);
                else if (blend == 2) channel = clamp(old - channel, 0, 31);
                else channel = clamp(old + channel / 4, 0, 31);
            }
            pixel |= (uint32_t)(channel * 255 / 31) << shift;
        }
        out[y * W + x] = pixel;
    }
}

void bfm_stair_actors_draw_to(BfmStairActors *a, int depth,
                             const uint16_t tiles[512][1024], uint32_t *pixels)
{
    while (a && a->next < a->used && a->order[a->next].depth >= depth) {
        const Draw *d = &a->order[a->next++];
        draw_triangle(d, tiles, pixels, 0, 1, 2);
        if (d->face->count == 4) draw_triangle(d, tiles, pixels, 2, 1, 3);
    }
}

/* The shadow is a square from the scene texture upload. It lies in the
 * local tangent plane under the walking figure and turns with its heading. */
static void shoe_shadow(BfmStairActors *a, Vec root, int yaw, Mat view, Vec translation)
{
    Vec n; int y;
    if (a->used == MAX_DRAW || !ground(a, root, &n, &y)) return;
    double to_angle = 4096.0 / 6.2831853071795864769;
    int x = (int)lround(atan2(n.v[1], -n.v[2]) * to_angle);
    int bearing = (int)lround(atan2(n.v[0], -hypot(n.v[1], n.v[2])) * to_angle);
    Mat plane = multiply(multiply(rotation(0, x), rotation(1, bearing)), rotation(2, -yaw));
    Mat axes = multiply(view, plane);
    root.v[1] = y - 2;
    Vec center = add(translation, transform(view, root));
    Vec opposite = {{-n.v[0],-n.v[1],-n.v[2]}};
    Vec light = transform(stage_directions, opposite);
    for (unsigned k = 0; k < 3; ++k) light.v[k] = clamp(light.v[k], 0, 32767);
    Vec shade = add((Vec){{2048,2048,2048}}, transform(stage_colors, light));
    for (unsigned k = 0; k < 3; ++k) shade.v[k] = clamp(shade.v[k] / 32, 0, 255);
    Draw *d = a->order + a->used;
    for (unsigned j = 0; j < 4; ++j) {
        int u = a->shadow_left + a->shadow.uv[j][0] - 64 + ((j & 1) ? 1 : 0);
        int v = a->shadow_top + a->shadow.uv[j][1] - 160 + ((j & 2) ? 1 : 0);
        Vec point = add(center, transform(axes, (Vec){{3*u,3*v,0}}));
        if (point.v[2] <= 0) return;
        d->point[j] = project(point);
        d->shade[j] = shade;
    }
    d->depth = center.v[2] / 4 + 2;
    d->face = &a->shadow;
    d->sequence = (unsigned)a->used++;
}

/* Loose masonry falls under a common acceleration. At the stair collision
 * it rebounds and turns about two axes until the small second impact. */
static void masonry(BfmStairActors *a, unsigned field, Mat view, Vec translation)
{
    if (field < 1478) return;
    static const struct { int dx, dz, clearance, settle_x, settle_y; } contact[] = {
        {3,0,17,1,16}, {7,0,15,1,16}, {6,2,16,1,16}
    };
    unsigned tick = (field - 1478) / 2;
    for (unsigned i = 0; i < 3; ++i) {
        int spin = 0, level;
        Vec center = a->spawn[i], normal;
        if (!ground(a, center, &normal, &level)) continue;
        level -= contact[i].clearance;
        unsigned hit = 0;
        while (hit < 256 && center.v[1] + (int)((hit+1)*(hit+2)) < level) ++hit;
        if (tick < hit) {
            int elapsed = (int)tick + 1;
            center.v[1] += elapsed * (elapsed + 1);
        } else {
            unsigned elapsed = tick - hit;
            spin = 256 * (int)(elapsed < 13 ? elapsed : 13);
            center.v[0] += contact[i].dx;
            center.v[2] += contact[i].dz;
            if (elapsed < 13) center.v[1] = level + (int)elapsed * ((int)elapsed - 11);
            else {
                center.v[0] += contact[i].settle_x;
                center.v[1] = level + contact[i].settle_y;
            }
        }
        Mat turn = angles((Vec){{spin,0,spin}});
        prepare_model(a, &a->parts[0], multiply(view, turn),
                      add(translation, transform(view, center)), turn);
    }
}

/* The princess turns toward the landing and takes a shorter second path.
 * The steward and butler finish their first path and settle into idle poses. */
static void actor_keys(const BfmStairActors *a, unsigned actor, unsigned tick,
                       const uint8_t **root, const uint8_t **bones)
{
    const Performer *p = a->actor + actor;
    unsigned path = tick < ROOT_POSES ? tick : ROOT_POSES - 1;
    unsigned pose = tick % p->frames;
    const uint8_t *poses = p->bones;
    *root = p->roots + 8*path;
    if (actor == 0 && tick >= 504) {
        unsigned landing = tick - 504;
        if (landing) *root = a->princess_landing + 8*(landing < 101 ? landing-1 : 99);
        else *root = p->roots + 8*503;
        poses = p->landing_bones;
        pose = landing < 14 ? landing : 13;
    } else if (actor && tick >= 600) {
        unsigned rest = tick - 600;
        poses = p->landing_bones;
        pose = actor == 2 ? rest % 10 : (rest < 23 ? rest : 22);
    }
    *bones = poses + 12*p->count*pose;
}

void bfm_stair_actors_prepare(BfmStairActors *a, unsigned field,
                              const int axes[3][3], const int origin[3])
{
    if (!a) return;
    a->used = 0; a->next = 0;
    Mat view; memcpy(view.a, axes, sizeof(view.a));
    Vec translation = {{origin[0],origin[1],origin[2]}};
    masonry(a, field, view, translation);
    if (field >= 1510) {
        unsigned tick = (field - 1510) / 2;
        if (tick) for (unsigned i = 0; i < ACTORS; ++i) {
            const Performer *p = a->actor + i;
            const uint8_t *root, *frame;
            actor_keys(a, i, tick, &root, &frame);
            int heading = signed_word(root + 6);
            shoe_shadow(a, triple(root), heading, view, translation);
            Mat yaw = rotation(1, heading), turned = multiply(view, yaw);
            Vec base = add(translation, transform(view, triple(root)));
            for (unsigned j = 0; j < p->count; ++j) {
                const uint8_t *bone = frame + 12 * j;
                Mat pose = angles(triple(bone + 6));
                Vec position = add(base, transform(turned, triple(bone)));
                prepare_model(a, a->parts + p->first + j, multiply(turned, pose), position, multiply(yaw, pose));
            }
        }
    }
    qsort(a->order, a->used, sizeof(*a->order), farthest);
}
