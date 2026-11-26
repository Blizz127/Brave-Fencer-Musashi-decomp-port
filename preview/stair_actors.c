/* SPDX-License-Identifier: MIT */
#include "stair_actors.h"
#include "forest_blade.h"
#include "ritual_light.h"
#include "musashi_forest_player_mesh.h"
#include "hero_smoke.h"
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { ACTORS = 5, PARTS = 41, MAX_VERTS = 128, MAX_FACES = 128,
       MAX_DRAW = 2048, MAX_RITUAL_DRAW = 256, ROOT_POSES = 601,
       BUTLER_REACTION_POSES = 32, W = 320, H = 240 };
typedef struct { int v[3]; } Vec;
typedef struct { int a[3][3]; } Mat;
typedef struct {
    uint16_t vertex[4], normal[4], palette, page;
    uint8_t uv[4][2], rgb[4][3], count, textured, lit, blend, summoned;
} Face;
typedef struct {
    Vec vertices[MAX_VERTS], normals[MAX_VERTS];
    unsigned nv, nn, nf;
    Face faces[MAX_FACES];
} Model;
typedef struct { const uint8_t *data; unsigned count; } Journey;
typedef struct { const uint8_t *frames; unsigned count; int repeat; } CourtMotion;

enum { HERO_WAKE = 6674, HERO_LEAP = 6876, HERO_TURN = 7082,
       HERO_FILLET_CLOSE = 7406, HERO_COURT_WIDE = 7774,
       HERO_SHEATHE_START = 14636, HERO_COURT_EXIT = 16426 };
enum HeroExpression { HERO_NEUTRAL_FACE, HERO_STARTLED_FACE, HERO_SLEEPING_FACE };
/* The close protest frames the adults. The following wide shot puts the
 * hero back beside them. Position, heading and shadow use the same mark. */
static const struct { unsigned field, descriptor, count; } hero_paths[] = {
    {HERO_LEAP, 0xab044, 60}, {HERO_TURN, 0xab7cc, 21},
    {8850, 0xac2e4, 2}, {9196, 0xac2f8, 2},
    {9590, 0xac30c, 205},          /* Musashi answers and turns. */
    {10032, 0xac978, 745},         /* Follow the passing courtier. */
    {11304, 0xae0c4, 2},           /* Face the court for his retort. */
    {11924, 0xae0d8, 249},         /* Turn with the conversation. */
    {12896, 0xaf2a8, 2},           /* Brace to raise Fusion. */
    {13118, 0xab01c, 2}, {14104, 0xab01c, 2},
    {14302, 0xaf2a8, 2},           /* Return to the raised stance. */
    {14704, 0xaf2e4, 20}, {14950, 0xaf388, 20},
    {15104, 0xaf42c, 2}, {15880, 0xaf440, 101} /* Turn, then leave the court. */
};
enum { HERO_PATHS = sizeof(hero_paths) / sizeof(hero_paths[0]) };

/* The palace actors advance one key every two displayed fields. Each entry
 * in the mounted list addresses a full pose of the shared player model. */
enum HeroMotion {
    HERO_RISE, HERO_YAWN, HERO_ROLL, HERO_DESCEND, HERO_LAND,
    HERO_STAND, HERO_GESTURE, HERO_HOLD_GESTURE, HERO_PRESENT_FUSION,
    HERO_SHEATHE, HERO_DEPART, HERO_MOTIONS
};
static const struct { unsigned descriptor, count; } hero_motions[HERO_MOTIONS] = {
    [HERO_RISE]         = {0x5f438, 12}, [HERO_YAWN]         = {0x5f1e0, 4},
    [HERO_ROLL]         = {0x5f2d8, 8},  [HERO_DESCEND]      = {0x5f678, 3},
    [HERO_LAND]         = {0x5f280, 10}, [HERO_STAND]        = {0x5f180, 11},
    [HERO_GESTURE]      = {0x5f528, 26}, [HERO_HOLD_GESTURE] = {0x5f600, 14},
    [HERO_PRESENT_FUSION] = {0x5f320, 1}, [HERO_SHEATHE] = {0x5f330, 32},
    [HERO_DEPART]       = {0x5f4a0, 16}
};
static const struct { unsigned field; enum HeroMotion motion; unsigned phase; } hero_actions[] = {
    {HERO_WAKE, HERO_RISE, 0}, {6698, HERO_YAWN, 0},
    {HERO_LEAP, HERO_ROLL, 7}, {6958, HERO_DESCEND, 2}, {6996, HERO_LAND, 0},
    {7080, HERO_STAND, 0}, {8088, HERO_GESTURE, 0}, {8140, HERO_HOLD_GESTURE, 0},
    {8324, HERO_STAND, 0},
    {9588, HERO_GESTURE, 0}, {9640, HERO_HOLD_GESTURE, 0}, {10030, HERO_STAND, 0},
    {11304, HERO_GESTURE, 0}, {11356, HERO_HOLD_GESTURE, 0}, {11796, HERO_STAND, 0},
    {12894, HERO_PRESENT_FUSION, 0}, {HERO_SHEATHE_START, HERO_SHEATHE, 0},
    {14702, HERO_STAND, 0}, {15878, HERO_DEPART, 0}
};
enum { HERO_ACTIONS = sizeof(hero_actions) / sizeof(hero_actions[0]) };
enum { MAX_HERO_POSES = 32 };
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
    BfmRitualNodes lights_on_crystal;
    BfmForestBlade fusion;
    BfmHeroSmokeMaterial smoke;
    Face smoke_faces[BFM_HERO_SMOKE_CELLS];
    Mat lights[3], selected_light;
    FloorSurface floor[512];
    size_t floor_count;
    Vec spawn[3];
    Face shadow;
    int shadow_left, shadow_top;
    Model parts[PARTS], crystal, hero[MUSASHI_FOREST_PLAYER_PARTS];
    Performer actor[ACTORS];
    const uint8_t *princess_landing;
    Journey arrival[ACTORS], step_forward[2], gathered[ACTORS], retreat[ACTORS];
    Journey question_paths[3];
    CourtMotion question, face_hero, answer[2], fillet_reply;
    Journey greetings[3][ACTORS], request_paths[3][ACTORS];
    Journey fillet_hold, steward_hold, scribe_cross;
    Journey departure_paths[8][ACTORS];
    CourtMotion fillet_appeal, fillet_farewell;
    Journey fusion_paths[6][ACTORS - 1];
    CourtMotion butler_offer, butler_raise;
    const uint8_t *princess_turn, *princess_speech, *princess_cast, *steward_retreat;
    const uint8_t *butler_reaction, *hero_pose;
    const uint8_t *hero_frames[HERO_MOTIONS][MAX_HERO_POSES];
    uint8_t hero_repeats[HERO_MOTIONS];
    Journey hero_roots[HERO_PATHS];
    enum HeroExpression hero_expression;
    unsigned fillet_expression;
    Draw order[MAX_DRAW + MAX_RITUAL_DRAW];
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

static int valid_texture(unsigned page, unsigned palette)
{
    unsigned count = page & 128u ? 256u : 16u;
    return ((page >> 7) & 3u) <= 1 && (palette >> 6) < 512 &&
           (palette & 63u) * 16 + count <= 1024;
}

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
        if (!valid_texture(f->page, f->palette)) return 0;
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
    m->nv = nv; m->nn = nn; m->nf = nf;
    for (unsigned i = 0; i < nv; ++i) m->vertices[i] = triple(s + vo + 8 * i);
    for (unsigned i = 0; i < nn; ++i) m->normals[i] = triple(s + no + 8 * i);
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

static Vec player_vector(MusashiForestPlayerVector v) { return (Vec){{v.x, v.y, v.z}}; }

/* The court and forest summon the same resident model. Keep its mounted
 * topology and materials, then use the court's lighting and ordering table. */
static int load_summoned_hero(BfmStairActors *a, const MusashiDiscMedia *disc, unsigned total_faces)
{
    MusashiForestPlayerMesh *mesh = musashi_forest_player_mesh_load(disc);
    if (!mesh) return 0;
    for (unsigned i = 0; i < MUSASHI_FOREST_PLAYER_PARTS; ++i) {
        const MusashiForestPlayerPart *p = musashi_forest_player_mesh_part(mesh, i);
        if (!p || p->vertex_count > MAX_VERTS || p->normal_count > MAX_VERTS ||
            p->face_count > MAX_FACES || p->face_count > MAX_DRAW - total_faces) goto fail;
        Model *m = a->hero + i;
        m->nv = (unsigned)p->vertex_count;
        m->nn = (unsigned)p->normal_count;
        m->nf = (unsigned)p->face_count;
        total_faces += m->nf;
        for (unsigned j = 0; j < m->nv; ++j) m->vertices[j] = player_vector(p->vertices[j]);
        for (unsigned j = 0; j < m->nn; ++j) m->normals[j] = player_vector(p->normals[j]);
        for (unsigned j = 0; j < m->nf; ++j) {
            const MusashiForestPlayerFace *src = p->faces + j;
            Face *f = m->faces + j;
            f->count = src->corners;
            f->lit = 1;
            f->summoned = 1;
            f->textured = (src->mode & 4u) != 0;
            f->palette = src->clut;
            f->page = src->tpage;
            if (f->textured && !valid_texture(f->page, f->palette)) goto fail;
            for (unsigned k = 0; k < f->count; ++k) {
                f->vertex[k] = src->vertex[k];
                f->normal[k] = src->normal[k];
                f->uv[k][0] = src->u[k];
                f->uv[k][1] = src->v[k];
                memcpy(f->rgb[k], src->rgb, sizeof(src->rgb));
            }
        }
    }
    musashi_forest_player_mesh_free(mesh);
    return 1;
fail:
    musashi_forest_player_mesh_free(mesh);
    return 0;
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
    const size_t light_tables[] = {0x609bc, 0x609dc, 0x609fc};
    for (unsigned profile = 0; profile < sizeof(light_tables) / sizeof(*light_tables); ++profile) {
        if (!within(bytes, light_tables[profile], 30)) return 0;
        for (unsigned lamp = 0; lamp < 3; ++lamp) {
            const uint8_t *color = s + light_tables[profile] + 10*lamp;
            for (unsigned component = 0; component < 3; ++component)
                a->lights[profile].a[component][lamp] = color[component] * 4096 / 255;
        }
    }
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

/* A new scene event selects a fresh world track. Its signed X terminator
 * ends movement; subsequent animation keeps the final position. */
static int read_journey_limited(Journey *out, const uint8_t *scene, size_t bytes,
                                size_t offset, unsigned max_count)
{
    if (!within(bytes, offset, 8)) return 0;
    for (unsigned i = 0; i <= max_count; ++i) {
        size_t pos = offset + 8*i;
        if (!within(bytes, pos, 2)) return 0;
        if (signed_word(scene + pos) == -32768) {
            if (!i) return 0;
            *out = (Journey){scene + offset, i};
            return 1;
        }
        if (!within(bytes, pos, 8)) return 0;
    }
    return 0;
}

static int read_journey(Journey *out, const uint8_t *scene, size_t bytes, size_t offset)
{
    return read_journey_limited(out, scene, bytes, offset, 600);
}

/* The court's motion lists reference packed local poses. Each entry lasts
 * one scene update. The terminator either repeats the motion or holds its
 * final pose while the next line plays. */
static int read_court_motion(CourtMotion *out, const uint8_t *s, size_t bytes,
                             size_t table, unsigned bones)
{
    const uint32_t origin = UINT32_C(0x80128158);
    size_t stride = 12 * bones, first = 0;
    for (unsigned i = 0; within(bytes, table, 8); ++i, table += 8) {
        uint32_t address = longword(s + table), duration = longword(s + table + 4);
        if (!address) {
            if (!i || (duration != 0xfffe && duration != 0xffff)) return 0;
            *out = (CourtMotion){s + first, i, duration == 0xffff};
            return 1;
        }
        if (address < origin || duration != 1) return 0;
        size_t pos = address - origin;
        if (!within(bytes, pos, stride)) return 0;
        if (!i) first = pos;
        else if (pos != first + i * stride) return 0;
    }
    return 0;
}

static int load_hero_actions(BfmStairActors *a, const uint8_t *scene, size_t bytes, uint32_t load_address)
{
    const size_t pose_bytes = MUSASHI_FOREST_PLAYER_PARTS * 12;
    if (!within(bytes, 0x8a6d4, pose_bytes)) return 0;
    for (unsigned i = 0; i < HERO_PATHS; ++i) {
        if (!read_journey_limited(&a->hero_roots[i], scene, bytes,
                                  hero_paths[i].descriptor, hero_paths[i].count) ||
            a->hero_roots[i].count != hero_paths[i].count) return 0;
    }
    for (unsigned i = 0; i < HERO_MOTIONS; ++i) {
        unsigned pos = hero_motions[i].descriptor, count = hero_motions[i].count;
        if (count > MAX_HERO_POSES || !within(bytes, pos, (count + 1) * 8u)) return 0;
        const uint8_t *list = scene + pos;
        for (unsigned j = 0; j < count; ++j) {
            uint32_t ptr = longword(list + j * 8u);
            if (ptr < load_address || !within(bytes, ptr - load_address, pose_bytes) ||
                longword(list + j * 8u + 4) != 1) return 0;
            a->hero_frames[i][j] = scene + (ptr - load_address);
        }
        const uint8_t *end = list + count * 8u;
        if (longword(end) || (longword(end + 4) != 0xfffe && longword(end + 4) != 0xffff))
            return 0;
        a->hero_repeats[i] = (uint8_t)(longword(end + 4) == 0xffff);
    }
    a->hero_pose = scene + 0x8a6d4;
    return 1;
}

static int load_smoke(BfmStairActors *actors, const uint8_t *scene, size_t bytes)
{
    if (!bfm_hero_smoke_load(&actors->smoke, scene, bytes)) return 0;
    for (unsigned cell = 0; cell < BFM_HERO_SMOKE_CELLS; ++cell) {
        const BfmHeroSmokeFrame *frame = &actors->smoke.tracks[0][cell];
        Face *face = &actors->smoke_faces[cell];
        face->page = frame->page;
        face->palette = frame->palette;
        face->count = 4;
        face->textured = 1;
        face->blend = 1;
        memcpy(face->uv, frame->uv, sizeof(face->uv));
    }
    return 1;
}

BfmStairActors *bfm_stair_actors_load(const MusashiDiscMedia *disc, const uint8_t *s, size_t bytes)
{
    /* The US stairwell has one loose rock and three entering models.
     * Pointer lists refer to the member's original load address. Pose frames
     * store a local translation and three angles per piece, independent of
     * the scene track which moves the whole figure. */
    static const struct { unsigned list, bones, root, count, frames, landing_bones, landing_frames; } layout[ACTORS] = {
        {0x73be0, 0x7cff4, 0xaf764, 8, 20, 0x7d774, 14},
        {0x7634c, 0x823c4, 0xb3d4c, 10, 12, 0x82964, 30},
        {0x7848c, 0x86b7c, 0xbe8e4, 7, 12, 0x86834, 10},
        {0x7a2a4, 0x8774c, 0, 8, 12, 0x881cc, 24},
        {0x7cfd4, 0x88acc, 0, 7, 32, 0x8954c, 25}
    };
    const uint32_t load_address = UINT32_C(0x80128158);
    const unsigned rock = 0x71604u;
    if (!s || !within(bytes, rock, 40)) return NULL;
    BfmStairActors *out = calloc(1, sizeof(*out));
    if (!out || !bfm_palace_blade_load(&out->fusion, s, bytes) || !read_effects(out, s, bytes) || !load_model(&out->parts[0], s + rock, bytes - rock)) goto fail;
    if (!within(bytes, 0xb0a30, 100*8)) goto fail;
    out->princess_landing = s + 0xb0a30;
    static const unsigned arrival[ACTORS] = {0xb0d54, 0xb5018, 0xbfbb0, 0xbc8fc, 0xb8724};
    static const unsigned gathered[ACTORS] = {0xb1208, 0xb54cc, 0xc0064, 0xbcdc4, 0xb874c};
    static const unsigned retreat[ACTORS] = {0xb1b80, 0xb54f4, 0xc008c, 0xbcdec, 0xb8774};
    if (!within(bytes, 0x7e434, 3 * 8 * 12) ||
        !within(bytes, 0x7dcb4, 20 * 8 * 12) ||
        !within(bytes, 0x7e914, 8 * 8 * 12) ||
        !within(bytes, 0x81f14, 10 * 10 * 12) ||
        !within(bytes, 0x84a34, BUTLER_REACTION_POSES * layout[1].count * 12)) goto fail;
    out->princess_turn = s + 0x7e434;
    out->princess_speech = s + 0x7dcb4;
    out->princess_cast = s + 0x7e914;
    out->steward_retreat = s + 0x81f14;
    out->butler_reaction = s + 0x84a34;
    if (!read_journey(&out->question_paths[0], s, bytes, 0xb1c7c) ||
        !read_journey(&out->question_paths[1], s, bytes, 0xc0108) ||
        !read_journey(&out->question_paths[2], s, bytes, 0xb87f0) ||
        !read_court_motion(&out->question, s, bytes, 0x5f9c0, layout[0].count) ||
        !read_court_motion(&out->face_hero, s, bytes, 0x5fd08, layout[0].count) ||
        !read_court_motion(&out->answer[0], s, bytes, 0x60410, layout[2].count) ||
        !read_court_motion(&out->answer[1], s, bytes, 0x60870, layout[4].count)) goto fail;
    if (!read_journey(&out->step_forward[0], s, bytes, 0xbc910) ||
        !read_journey(&out->step_forward[1], s, bytes, 0xb8738)) goto fail;
    static const unsigned greetings[3][ACTORS] = {
        {0xb29fc, 0xb5570, 0xc0a6c, 0xbce68, 0xb9154},
        {0xb2fe8, 0xb5b5c, 0xc0a80, 0xbce7c, 0xb9740},
        {0xb33ac, 0xb5b70, 0xc0e44, 0xbce90, 0xb9754}
    };
    for (unsigned line = 0; line < 3; ++line)
        for (unsigned i = 0; i < ACTORS; ++i)
            if (!read_journey(&out->greetings[line][i], s, bytes, greetings[line][i])) goto fail;
    static const unsigned request_paths[3][ACTORS] = {
        {0xb33c0, 0xb5b84, 0xc0e58, 0xbcea4, 0xb9768},
        {0,       0xb5b98, 0xc0e6c, 0xbceb8, 0xb977c},
        {0,       0xb60cc, 0xc13a0, 0xbcecc, 0xb9790}
    };
    for (unsigned line = 0; line < 3; ++line) {
        for (unsigned i = 0; i < ACTORS; ++i) {
            unsigned offset = request_paths[line][i];
            if (offset && !read_journey(&out->request_paths[line][i], s, bytes, offset)) goto fail;
        }
    }
    static const unsigned fusion_paths[][ACTORS - 1] = {
        {0xb68d0, 0xc1bb8, 0xbcee0, 0xbaef0},
        {0xb709c, 0xc22d4, 0xbd6ac, 0xbb6bc},
        {0xb7aa0, 0xc22e8, 0xbe0b0, 0xbc0c0},
        {0xb7dc4, 0xc22fc, 0xbe3d4, 0xbc3e4},
        {0xb7f60, 0xc2310, 0xbe3e8, 0xbc3f8},
        {0xb7f74, 0xc2324, 0xbe3fc, 0xbc40c}
    };
    for (unsigned stage = 0; stage < sizeof(fusion_paths) / sizeof(*fusion_paths); ++stage)
        for (unsigned i = 0; i < ACTORS - 1; ++i)
            if (!read_journey(&out->fusion_paths[stage][i], s, bytes, fusion_paths[stage][i])) goto fail;
    if (!read_court_motion(&out->butler_offer, s, bytes, 0x5fff0, layout[1].count) ||
        !read_court_motion(&out->butler_raise, s, bytes, 0x60078, layout[1].count)) goto fail;
    static const unsigned departure_paths[][ACTORS] = {
        {0,       0xb7f88, 0xc2338, 0xbe410, 0xbc420},
        {0xb3488, 0,       0,       0,       0},
        {0xb3534, 0xb7f9c, 0xc234c, 0xbe424, 0xbc434},
        {0xb3808, 0xb8270, 0xc2360, 0xbe438, 0xbc448},
        {0xb3954, 0xb8284, 0xc2374, 0xbe44c, 0xbc45c},
        {0xb3968, 0xb8710, 0xc26a0, 0xbe8d8, 0xbc8e8},
        {0xb397c, 0,       0,       0,       0},
        {0xb39e8, 0,       0xc26b4, 0,       0}
    };
    for (unsigned i = 0; i < sizeof(departure_paths) / sizeof(*departure_paths); ++i)
        for (unsigned j = 0; j < ACTORS; ++j) {
            unsigned offset = departure_paths[i][j];
            if (offset && !read_journey(&out->departure_paths[i][j], s, bytes, offset)) goto fail;
        }
    if (!read_court_motion(&out->fillet_appeal, s, bytes, 0x5fa68, layout[0].count) ||
        !read_court_motion(&out->fillet_farewell, s, bytes, 0x5faf0, layout[0].count)) goto fail;
    enum { SCRIBE_CROSS_KEYS = 745 };
    if (!read_journey(&out->fillet_hold, s, bytes, 0xb33fc) ||
        !read_journey(&out->steward_hold, s, bytes, 0xc1ba4) ||
        !read_journey_limited(&out->scribe_cross, s, bytes, 0xb97a4, SCRIBE_CROSS_KEYS) ||
        out->scribe_cross.count != SCRIBE_CROSS_KEYS) goto fail;
    if (!read_court_motion(&out->fillet_reply, s, bytes, 0x5fc38, layout[0].count)) goto fail;
    unsigned part = 1;
    if (!bfm_ritual_light_load(&out->lights_on_crystal, s, bytes) ||
        !load_smoke(out, s, bytes)) goto fail;
    const size_t crystal_model = 0x711d4;
    if (!within(bytes, crystal_model, 40) ||
        !load_model(&out->crystal, s + crystal_model, bytes - crystal_model)) goto fail;
    unsigned total_faces = 3 * out->parts[0].nf + ACTORS + out->crystal.nf;
    for (unsigned i = 0; i < ACTORS; ++i) {
        unsigned n = layout[i].count, frames = layout[i].frames;
        if (!read_journey(&out->arrival[i], s, bytes, arrival[i]) ||
            !read_journey(&out->gathered[i], s, bytes, gathered[i]) ||
            !read_journey(&out->retreat[i], s, bytes, retreat[i]) ||
            !within(bytes, layout[i].list, 4 * (n + 1)) ||
            !within(bytes, layout[i].bones, 12 * n * frames) ||
            !within(bytes, layout[i].landing_bones, 12 * n * layout[i].landing_frames) ||
            (i < 3 && !within(bytes, layout[i].root, ROOT_POSES * 8))) goto fail;
        const uint8_t *list = s + layout[i].list;
        if (longword(list + 4 * n)) goto fail;
        out->actor[i] = (Performer){s + layout[i].bones, i < 3 ? s + layout[i].root : NULL, s + layout[i].landing_bones, part, n, frames};
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
    if (total_faces > MAX_DRAW - 8 || !load_hero_actions(out, s, bytes, load_address) ||
        !load_summoned_hero(out, disc, total_faces + 8)) goto fail;
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

static void prepare_model(BfmStairActors *group, const Model *m, Mat model, Vec origin, Mat local)
{
    Mat lighting = multiply(stage_directions, local);
    Vec pts[MAX_VERTS], intensity[MAX_VERTS];
    for (unsigned i = 0; i < m->nv; ++i)
        pts[i] = project(add(origin, transform(model, m->vertices[i])));
    for (unsigned i = 0; i < m->nn; ++i) {
        Vec energy = transform(lighting, m->normals[i]);
        for (unsigned j = 0; j < 3; ++j) energy.v[j] = clamp(energy.v[j], 0, 32767);
        intensity[i] = add((Vec){{2048,2048,2048}}, transform(group->selected_light, energy));
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
        /* At a 256-unit projection distance, a vertex at 128 or closer
         * saturates the perspective divide. The court omits those faces. */
        if (near <= 128 || edge(d->point[0], d->point[1], d->point[2].v[0], d->point[2].v[1]) <= 0)
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

static const Face blade_light = {.page = 32, .count = 4, .blend = 1};

static int farthest(const void *one, const void *two)
{
    const Draw *a = one, *b = two;
    if (a->depth != b->depth) return a->depth > b->depth ? -1 : 1;
    if ((a->face == &blade_light) != (b->face == &blade_light))
        return a->face == &blade_light ? 1 : -1;
    return a->sequence > b->sequence ? -1 : a->sequence < b->sequence;
}

static uint16_t texture(const Face *face, const uint16_t tiles[512][1024], unsigned u, unsigned v,
                        enum HeroExpression hero_expression)
{
    unsigned bits = (face->page & 128) ? 8u : 4u, count = 16 / bits;
    unsigned row = (face->page & 16) * 16 + v;
    unsigned col = ((face->page & 15) * 64 + u / count) & 1023u;
    /* Neutral uses the model UVs. The startled and sleeping faces occupy
     * two adjacent rows in the mounted actor atlas. */
    if (face->summoned && hero_expression != HERO_NEUTRAL_FACE &&
        col >= 512 && col < 528 && row >= 256 && row < 296) {
        col += 256;
        if (hero_expression == HERO_SLEEPING_FACE) row += 40;
    }
    unsigned idx = (tiles[row][col] >> (bits * (u % count))) & ((1u << bits) - 1u);
    return tiles[face->palette >> 6][(face->palette & 63) * 16 + idx];
}

static int closed(Vec a, Vec b)
{
    return a.v[1] > b.v[1] || (a.v[1] == b.v[1] && a.v[0] < b.v[0]);
}
static void draw_triangle(const Draw *d, const uint16_t tiles[512][1024], uint32_t *out,
                          unsigned i, unsigned j, unsigned k, enum HeroExpression hero_expression)
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
            ink = texture(f, tiles, coord[0], coord[1], hero_expression);
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
                unsigned blend = (f->page >> 5) & 3u;
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

/* Rasterize along the dominant axis. On the positive-X span the first
 * shade starts at the left endpoint, including a reversed submission. */
static void draw_ritual_line(const Draw *d, uint32_t *pixels)
{
    Vec a = d->point[0], b = d->point[1];
    if (a.v[0] > b.v[0]) { Vec q = a; a = b; b = q; }
    int dx = b.v[0] - a.v[0], dy = -abs(b.v[1] - a.v[1]);
    int sy = a.v[1] < b.v[1] ? 1 : -1;
    int err = dx + dy, length = dx > -dy ? dx : -dy;
    if (dx > -dy) err -= (dx - dy - 1) / 2 - (dx - 1) / 2;
    for (int i = 0;; ++i) {
        if ((unsigned)a.v[0] < W && a.v[1] >= 10 && a.v[1] < H - 10) {
            uint32_t *dest = pixels + a.v[1] * W + a.v[0];
            uint32_t pixel = UINT32_C(0xff000000);
            for (unsigned k = 0; k < 3; ++k) {
                int shade = d->shade[0].v[k];
                if (length)
                    shade = (shade * (length - i) + d->shade[1].v[k] * i) / length;
                unsigned shift = 16 - 8 * k;
                unsigned old = (((*dest >> shift) & 255u) * 31 + 127) / 255;
                unsigned value = old + (unsigned)shade / 8;
                pixel |= (value >= 31 ? 255u : value * 255 / 31) << shift;
            }
            *dest = pixel;
        }
        if (a.v[0] == b.v[0] && a.v[1] == b.v[1]) break;
        int twice = 2 * err;
        if (twice > dy) { err += dy; ++a.v[0]; }
        if (twice < dx) { err += dx; a.v[1] += sy; }
    }
}

static void queue_ritual_light(void *context, unsigned count, const int xyz[4][3],
                               const unsigned rgb[4][3], int bucket)
{
    static const Face line = {.page = 32, .count = 2, .blend = 1};
    static const Face quad = {.page = 32, .count = 4, .blend = 1};
    BfmStairActors *a = context;
    if ((count != 2 && count != 4) || a->used >= MAX_DRAW + MAX_RITUAL_DRAW) return;
    Draw *d = a->order + a->used;
    d->face = count == 2 ? &line : &quad;
    d->depth = bucket;
    /* Light batches retain emission order ahead of an equal-depth mesh. */
    d->sequence = UINT_MAX - (unsigned)a->used++;
    for (unsigned i = 0; i < count; ++i) {
        for (unsigned k = 0; k < 3; ++k) {
            d->point[i].v[k] = xyz[i][k];
            d->shade[i].v[k] = (int)rgb[i][k];
        }
    }
}

static void queue_hero_smoke(void *context, unsigned field_age,
                             const int xyz[4][3], int bucket)
{
    BfmStairActors *actors = context;
    const BfmHeroSmokeFrame *frame = bfm_hero_smoke_frame(&actors->smoke, 0, field_age);
    if (!frame || actors->used >= MAX_DRAW + MAX_RITUAL_DRAW) return;
    Draw *draw = &actors->order[actors->used];
    draw->face = &actors->smoke_faces[frame - actors->smoke.tracks[0]];
    draw->depth = bucket;
    draw->sequence = UINT_MAX - (unsigned)actors->used++;
    for (unsigned i = 0; i < 4; ++i) {
        memcpy(draw->point[i].v, xyz[i], sizeof(draw->point[i].v));
        for (unsigned ch = 0; ch < 3; ++ch)
            draw->shade[i].v[ch] = 128;
    }
}

void bfm_stair_actors_draw_to(BfmStairActors *a, int depth,
                             const uint16_t tiles[512][1024], uint32_t *pixels)
{
    while (a && a->next < a->used && a->order[a->next].depth >= depth) {
        const Draw *d = &a->order[a->next++];
        if (d->face->count == 2) {
            draw_ritual_line(d, pixels);
            continue;
        }
        draw_triangle(d, tiles, pixels, 0, 1, 2, a->hero_expression);
        if (d->face->count == 4) draw_triangle(d, tiles, pixels, 2, 1, 3, a->hero_expression);
    }
}

/* The shadow is a square from the scene texture upload. It lies in the
 * local tangent plane under the walking figure and turns with its heading. */
static void shoe_shadow(BfmStairActors *a, Vec root, int yaw, int scale, Mat view, Vec translation,
                       int near_depth)
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
    Vec shade = add((Vec){{2048,2048,2048}}, transform(a->selected_light, light));
    for (unsigned k = 0; k < 3; ++k) shade.v[k] = clamp(shade.v[k] / 32, 0, 255);
    Draw *d = a->order + a->used;
    for (unsigned j = 0; j < 4; ++j) {
        int u = a->shadow_left + a->shadow.uv[j][0] - 64 + ((j & 1) ? 1 : 0);
        int v = a->shadow_top + a->shadow.uv[j][1] - 160 + ((j & 2) ? 1 : 0);
        Vec point = add(center, transform(axes, (Vec){{scale*u,scale*v,0}}));
        if (point.v[2] <= near_depth) return;
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

static const uint8_t *journey_pose(Journey path, unsigned progress)
{
    return path.data + 8 * (progress < path.count ? progress : path.count-1);
}

static const unsigned idle_frames[ACTORS] = {0, 30, 10, 24, 25};

/* Around the Princess's casting declaration the attendants step back. Their
 * pose cycles restart before the first translation and settle independently. */
static void retreat_keys(const BfmStairActors *a, unsigned actor, unsigned field,
                         const uint8_t **root, const uint8_t **bones)
{
    const Performer *p = a->actor + actor;
    Journey path = a->retreat[actor];
    unsigned age = (field - 4014) / 2;
    unsigned start = actor ? 4016 : 4018;
    *root = field < start ? journey_pose(a->gathered[actor], a->gathered[actor].count)
                          : journey_pose(path, (field - start) / 2);
    const uint8_t *motion = p->bones;
    unsigned pose;
    if (actor == 0) {
        if (!age) {
            motion = a->princess_turn;
            pose = 2;
        } else if (field < 4146) {
            motion = a->princess_speech;
            pose = age < 20 ? age - 1 : 19;
        } else {
            motion = a->princess_cast;
            unsigned cast = (field - 4146) / 2;
            pose = cast < 8 ? cast : 7;
        }
    } else if (actor == 1 && field >= 4634) {
        motion = a->butler_reaction;
        pose = ((field - 4634) / 2) % BUTLER_REACTION_POSES;
    } else if (actor != 3 && age >= path.count) {
        motion = p->landing_bones;
        pose = (age - path.count) % idle_frames[actor];
    } else if (actor == 1 || actor == 2) {
        motion = actor == 1 ? a->steward_retreat : p->landing_bones;
        pose = age % 10;
    } else {
        pose = age % p->frames;
    }
    *bones = motion + 12 * p->count * pose;
}

/* Fillet turns to ask the two attendants beside her about the real Musashi.
 * The attendants stop moving before they answer. Fillet continues her track,
 * pausing its cursor for one update each time their exchange changes motion. */
enum { QUESTION_FIELD = 5696 };

static void question_keys(const BfmStairActors *a, unsigned actor, unsigned field,
                          const uint8_t **root, const uint8_t **bones)
{
    enum { STEP = 5698, ATTENDANTS_STOP = 6286, ANSWER = 6288,
           ANSWER_END = 6402, FACE_HERO = 6404, RELAX = 6872 };
    const Performer *p = a->actor + actor;
    unsigned side = actor / 2;
    if (field >= STEP) {
        unsigned until = actor && field > ATTENDANTS_STOP ? ATTENDANTS_STOP : field;
        unsigned progress = (until - STEP) / 2;
        if (!actor && field >= ANSWER) --progress;
        if (!actor && field >= ANSWER_END) --progress;
        *root = journey_pose(a->question_paths[side], progress);
    }
    CourtMotion action;
    unsigned start;
    if (actor) {
        if (field < ATTENDANTS_STOP) return;
        if (field < ANSWER || field >= ANSWER_END) {
            unsigned pose = field < ANSWER ? 0 : (field - ANSWER_END) / 2 % idle_frames[actor];
            *bones = p->landing_bones + 12 * p->count * pose;
            return;
        }
        action = a->answer[side - 1];
        start = ANSWER;
    } else if ((field >= ANSWER && field < FACE_HERO) || field >= RELAX) {
        unsigned pose = (field - (field >= RELAX ? RELAX : ANSWER)) / 2 % p->frames;
        *bones = p->bones + 12 * p->count * pose;
        return;
    } else {
        action = field < ANSWER ? a->question : a->face_hero;
        start = field < ANSWER ? QUESTION_FIELD : FACE_HERO;
    }
    unsigned tick = (field - start) / 2;
    unsigned pose = action.repeat ? tick % action.count : (tick < action.count ? tick : action.count - 1);
    *bones = action.frames + 12 * p->count * pose;
}

enum { GREETING_FIELD = 7404, APOLOGY_FIELD = 8324 };

/* Ribson apologizes. He and the Butler then cross the room and turn back.
 * Their idle cycles and Shanky's restart on the last key of each
 * completed track. Fillet and Leanman keep their current body animations. */
static void request_keys(const BfmStairActors *a, unsigned actor, unsigned field,
                         const uint8_t **root, const uint8_t **bones)
{
    static const unsigned starts[] = {APOLOGY_FIELD, 8848, 9196};
    const Performer *p = &a->actor[actor];
    for (unsigned line = 0; line < sizeof(starts) / sizeof(*starts); ++line) {
        if (field < starts[line]) break;
        Journey path = a->request_paths[line][actor];
        if (!path.count) continue;
        unsigned tick = (field - starts[line]) / 2;
        *root = journey_pose(path, tick);
        if ((actor == 1 || actor == 2 || actor == 4) && tick >= path.count - 1) {
            unsigned idle = (tick - (path.count - 1)) % idle_frames[actor];
            *bones = p->landing_bones + 12 * p->count * idle;
        }
    }
}

/* Fillet holds her stance as Musashi protests; Ribson turns toward him.
 * Shanky then carries his book across the room. The final movement key
 * starts a fresh cycle of his standing animation. */
enum { PROTEST_FIELD = 9588 };
static void protest_keys(const BfmStairActors *a, unsigned actor, unsigned field,
                         const uint8_t **root, const uint8_t **bones)
{
    enum { FILLET_HOLD = 9590, STEWARD_HOLD = 9592, SCRIBE_STEP = 10032 };
    const Performer *p = &a->actor[actor];
    if (actor == 0 && field >= FILLET_HOLD) {
        *root = journey_pose(a->fillet_hold, (field - FILLET_HOLD) / 2);
    } else if (actor == 2) {
        unsigned idle = (field - PROTEST_FIELD) / 2;
        if (field >= STEWARD_HOLD) {
            unsigned step = (field - STEWARD_HOLD) / 2;
            *root = journey_pose(a->steward_hold, step);
            if (step >= a->steward_hold.count - 1)
                idle = step - (a->steward_hold.count - 1);
        }
        *bones = p->landing_bones + 12 * p->count * (idle % idle_frames[actor]);
    } else if (actor == 4 && field >= SCRIBE_STEP) {
        unsigned step = (field - SCRIBE_STEP) / 2;
        *root = journey_pose(a->scribe_cross, step);
        if (step >= a->scribe_cross.count - 1) {
            unsigned idle = (step - (a->scribe_cross.count - 1)) % idle_frames[actor];
            *bones = p->landing_bones + 12 * p->count * idle;
        }
    }
}

/* The courtiers assemble around Fusion. Livers stops partway along his
 * approach to offer the sword, then raises it as the group moves again. */
enum { FUSION_APPROACH = 11922 };
static void fusion_keys(const BfmStairActors *a, unsigned actor, unsigned field,
                        const uint8_t **root, const uint8_t **bones)
{
    enum { TURN = 12332, STEP = 12336, OFFER = 12674, RAISE = 12894 };
    if (!actor) return;
    unsigned onward = RAISE + (actor <= 2 ? 2u : 4u);
    unsigned stage = field < STEP ? 0 : field < onward ? 1 : 2;
    unsigned first = stage == 0 ? FUSION_APPROACH : stage == 1 ? STEP : onward;
    unsigned current = field;
    if (stage == 0 && current > TURN) current = TURN;
    if (stage == 1 && actor == 1 && current >= OFFER) current = OFFER - 2;
    if (stage == 1 && current > RAISE) current = RAISE;
    Journey path = a->fusion_paths[stage][actor - 1];
    unsigned step = (current - first) / 2;
    *root = journey_pose(path, step);
    if (actor == 3 || field < TURN) return;

    const Performer *p = &a->actor[actor];
    unsigned idle = TURN;
    if (actor == 1 && field >= OFFER) {
        unsigned finish = onward + 2 * (a->fusion_paths[2][0].count - 1);
        if (field < finish) {
            CourtMotion action = field < RAISE ? a->butler_offer : a->butler_raise;
            unsigned key = (field - (field < RAISE ? OFFER : RAISE)) / 2;
            if (action.repeat) key %= action.count;
            else if (key >= action.count) key = action.count - 1;
            *bones = action.frames + 12 * p->count * key;
            return;
        }
        idle = finish;
    } else if (actor == 2 && field >= STEP + 2) {
        idle = STEP + 2;
    } else if (actor == 4 && field >= RAISE) {
        idle = RAISE;
        if (stage == 2 && step >= path.count - 1) idle = onward + 2 * (path.count - 1);
    }
    *bones = p->landing_bones + 12 * p->count * (((field - idle) / 2) % idle_frames[actor]);
}

/* Livers steps back to let the court explain the sword. Each successive
 * mark settles the courtiers into their own standing cycles. */
static void explain_fusion_keys(const BfmStairActors *a, unsigned actor, unsigned field,
                                const uint8_t **root, const uint8_t **bones)
{
    static const unsigned starts[] = {13118, 14104, 14638};
    if (!actor) return;
    for (unsigned i = 0; i < sizeof(starts) / sizeof(*starts); ++i) {
        if (field < starts[i]) break;
        Journey path = a->fusion_paths[3 + i][actor - 1];
        unsigned tick = (field - starts[i]) / 2;
        *root = journey_pose(path, tick);
        if (actor != 3 && tick >= path.count - 1) {
            const Performer *p = &a->actor[actor];
            unsigned idle = (tick - (path.count - 1)) % idle_frames[actor];
            *bones = p->landing_bones + 12 * p->count * idle;
        }
    }
}

enum { FILLET_QUESTION = 14704, FILLET_STEP = 14948 };

/* Fillet steps into the group and turns. The cast advances a mounted world
 * key every two fields; a finished short track resets the standing cycle. */
static void departure_keys(const BfmStairActors *a, unsigned actor, unsigned field,
                           const uint8_t **root, const uint8_t **bones)
{
    enum { CAST_TURN = 16068, APPEAL = 16424, FAREWELL = 16812 };
    static const unsigned starts[] = {
        FILLET_QUESTION, 14882, 14950, 15104, 15880, CAST_TURN + 2, APPEAL + 2, FAREWELL + 2
    };
    const Performer *p = &a->actor[actor];
    for (unsigned i = 0; i < sizeof(starts) / sizeof(*starts); ++i) {
        unsigned start = starts[i];
        if ((i == 3 && actor <= 1) || (i == 5 && actor != 0)) start += 2;
        if (field < start) break;
        Journey path = a->departure_paths[i][actor];
        if (!path.count) continue;
        /* The old movement holds its last presented position while the
         * following track is being selected. */
        unsigned until = i == 4 && field > CAST_TURN ? CAST_TURN : field;
        unsigned tick = (until - start) / 2;
        *root = journey_pose(path, tick);
        if (actor != 0 && actor != 3 && tick >= path.count - 1) {
            unsigned idle = (tick - (path.count - 1)) % idle_frames[actor];
            *bones = p->landing_bones + 12 * p->count * idle;
        }
    }
    if (actor == 0 && field >= APPEAL) {
        CourtMotion gesture = field < FAREWELL ? a->fillet_appeal : a->fillet_farewell;
        unsigned key = (field - (field < FAREWELL ? APPEAL : FAREWELL)) / 2;
        if (gesture.repeat) key %= gesture.count;
        else if (key >= gesture.count) key = gesture.count - 1;
        *bones = gesture.frames + 12 * p->count * key;
    } else if (actor == 0 && field >= FILLET_STEP) {
        unsigned key = (field - FILLET_STEP) / 2 % p->frames;
        *bones = p->bones + 12 * p->count * key;
    } else if (actor == 1 && field >= 15102 && field < 15108) {
        *bones = p->landing_bones + 12 * p->count * ((field - 15102) / 2);
    } else if ((actor == 2 || actor == 4) && field >= CAST_TURN && field < CAST_TURN + 6) {
        *bones = p->landing_bones + 12 * p->count * ((field - CAST_TURN) / 2);
    }
}

/* The court turns when Musashi finishes his introduction. Short tracks
 * settle their attendants first, while Fillet pauses to speak and then turns
 * to the courtier who crosses the floor. */
static void greeting_keys(const BfmStairActors *a, unsigned actor, unsigned field,
                          const uint8_t **root, const uint8_t **bones)
{
    enum { ASK = 7604, REPLY = 7772, INTERCEDE = 8088 };
    const Performer *p = a->actor + actor;
    unsigned line = field >= INTERCEDE ? 2 : field >= REPLY ? 1 : 0;
    unsigned start;
    if (!line) start = actor == 3 ? GREETING_FIELD : GREETING_FIELD + 2;
    else if (line == 1) start = actor == 2 || actor == 3 ? REPLY + 2 : REPLY + 4;
    else start = INTERCEDE + 2;
    if (field >= start) {
        unsigned tick = (field - start) / 2;
        if (!line && actor == 0 && field >= ASK) tick = (ASK - 2 - start) / 2;
        *root = journey_pose(a->greetings[line][actor], tick);
    } else if (line == 1) {
        unsigned last = actor == 0 ? ASK - 2 : REPLY;
        unsigned previous = (last - (GREETING_FIELD + 2)) / 2;
        *root = journey_pose(a->greetings[0][actor], previous);
    } else if (line == 2) {
        *root = journey_pose(a->greetings[1][actor], a->greetings[1][actor].count);
    }
    if (!actor) {
        if (field < ASK) {
            unsigned tick = (field - GREETING_FIELD) / 2 % p->frames;
            *bones = p->bones + 12 * p->count * tick;
        } else {
            CourtMotion motion = field < REPLY + 2 ? a->question : a->fillet_reply;
            unsigned tick = (field - (field < REPLY + 2 ? ASK : REPLY + 2)) / 2;
            unsigned pose = motion.repeat ? tick % motion.count :
                                            (tick < motion.count ? tick : motion.count - 1);
            *bones = motion.frames + 12 * p->count * pose;
        }
    } else if (actor == 2) {
        unsigned crossing_end = REPLY + 2 * a->greetings[1][actor].count;
        start = GREETING_FIELD + 4;
        if (field >= crossing_end) start = crossing_end;
        if (field >= INTERCEDE + 4) start = INTERCEDE + 4;
        if (field >= start)
            *bones = p->landing_bones + 12 * p->count * ((field - start) / 2 % idle_frames[actor]);
    } else if ((actor == 1 || actor == 4) && field >= REPLY) {
        start = REPLY;
        if (field >= REPLY + 6) start = REPLY + 6;
        if (field >= INTERCEDE + 4) start = INTERCEDE + 4;
        *bones = p->landing_bones + 12 * p->count * ((field - start) / 2 % idle_frames[actor]);
    }
}

/* The court gathers beside the crystal, then the two keepers turn to speak.
 * A new movement selects the walking cycle before its first translation.
 * Attendants return to their own idle cycle at the end of a short path. */
static int crystal_keys(const BfmStairActors *a, unsigned actor, unsigned field,
                        const uint8_t **root, const uint8_t **bones)
{
    const Performer *p = a->actor + actor;
    if (field >= 4014) {
        retreat_keys(a, actor, field, root, bones);
        if (field >= QUESTION_FIELD && (actor == 0 || actor == 2 || actor == 4))
            question_keys(a, actor, field, root, bones);
        if (field >= GREETING_FIELD) greeting_keys(a, actor, field, root, bones);
        if (field >= APOLOGY_FIELD) request_keys(a, actor, field, root, bones);
        if (field >= PROTEST_FIELD) protest_keys(a, actor, field, root, bones);
        if (field >= FUSION_APPROACH) fusion_keys(a, actor, field, root, bones);
        if (field >= 13118) explain_fusion_keys(a, actor, field, root, bones);
        if (field >= FILLET_QUESTION) departure_keys(a, actor, field, root, bones);
        return 1;
    }
    unsigned cue = 2756, start = 2758;
    Journey path = a->arrival[actor];
    if (field >= 3170) { cue = 3170; start = 3172; path = a->gathered[actor]; }
    else if (actor >= 3 && field >= 2852) {
        cue = 2852; start = 2854; path = a->step_forward[actor-3];
    }
    /* Fillet leaves the old camera before the first crystal-wide draw. */
    if (actor == 0 && field >= 2758 && field < 2760) return 0;
    if (field < start) {
        if (cue == 2756) {
            if (actor >= 3) return 0;
            if (actor == 0) *root = a->princess_landing + 8*99;
            else *root = p->roots + 8*600;
        } else {
            Journey previous = actor >= 3 && cue == 3170 ? a->step_forward[actor-3] : a->arrival[actor];
            *root = journey_pose(previous, previous.count);
        }
    } else *root = journey_pose(path, (field-start)/2);
    unsigned age = (field-cue)/2;
    const uint8_t *motion = p->bones;
    unsigned cycle = p->frames;
    int holding = age >= path.count;
    if (actor && (actor != 3 || cue == 2756) && (holding || (actor >= 3 && cue == 2756))) {
        motion = p->landing_bones;
        cycle = idle_frames[actor];
        if (holding && !(actor == 3 && cue == 2756)) age -= path.count;
    }
    *bones = motion + 12*p->count*(age%cycle);
    if (!actor && field >= 4010)
        *bones = a->princess_turn + 12 * p->count * ((field - 4010) / 2);
    return 1;
}

/* Fillet first frowns at the costume, then keeps her mouth open while the
 * courtier answers. All three expressions share her head mesh and palette.
 * Select the mounted face rectangle once when the expression changes. */
static void fillet_expression(BfmStairActors *a, unsigned field)
{
    static const struct { unsigned page, u, v; } tile[] = {
        {0x1a, 0, 136}, {0x1c, 64, 224}, {0x1c, 128, 192}, {0x1c, 160, 192}
    };
    /* The walking princess relaxes her brow. During her appeal she uses
     * the small smiling mouth, then returns to the neutral face. */
    static const struct { unsigned field, tile; } changes[] = {
        {7494, 1}, {7614, 2}, {14950, 0}, {16430, 3}, {16814, 0}
    };
    unsigned next = 0;
    for (unsigned i = 0; i < sizeof(changes) / sizeof(*changes); ++i) {
        if (field < changes[i].field) break;
        next = changes[i].tile;
    }
    unsigned previous = a->fillet_expression;
    if (next == previous) return;
    Model *head = &a->parts[a->actor[0].first];
    for (unsigned i = 0; i < head->nf; ++i) {
        Face *f = &head->faces[i];
        if (!f->textured || f->palette != 0x55d6 || f->page != tile[previous].page) continue;
        unsigned j = 0;
        for (; j < f->count; ++j) {
            if ((unsigned)f->uv[j][0] - tile[previous].u >= 32 ||
                (unsigned)f->uv[j][1] - tile[previous].v >= 32) break;
        }
        if (j != f->count) continue;
        f->page = (uint16_t)tile[next].page;
        for (j = 0; j < f->count; ++j) {
            f->uv[j][0] = (uint8_t)(tile[next].u + f->uv[j][0] - tile[previous].u);
            f->uv[j][1] = (uint8_t)(tile[next].v + f->uv[j][1] - tile[previous].v);
        }
    }
    a->fillet_expression = next;
}

static const uint8_t *summoned_pose(const BfmStairActors *a, unsigned field)
{
    if (field < HERO_WAKE) return a->hero_pose;
    unsigned action = 0;
    while (action + 1 < HERO_ACTIONS && field >= hero_actions[action + 1].field) ++action;
    enum HeroMotion motion = hero_actions[action].motion;
    unsigned count = hero_motions[motion].count;
    unsigned frame = (field - hero_actions[action].field) / 2 + hero_actions[action].phase;
    if (a->hero_repeats[motion]) frame %= count;
    else if (frame >= count) frame = count - 1;
    return a->hero_frames[motion][frame];
}

/* The palace poses carry Fusion on model piece 22. Its light follows the
 * blade axis as the hand turns it, facing the eye independently of the grip. */
static void prepare_fusion_light(BfmStairActors *a, unsigned tick, const int eye[3],
                                 Vec grip, Mat blade, Mat view, Vec translation)
{
    /* Center 29 model units down the shaft. The mounted edge's 48-unit
     * span supplies the two samples defining its longitudinal direction. */
    static const int8_t axis[2][3] = {{0, 0, -53}, {0, 0, -5}};
    BfmForestTransform mount, glow;
    for (unsigned i = 0; i < 3; ++i) {
        mount.translation[i] = grip.v[i];
        for (unsigned j = 0; j < 3; ++j) mount.rotation[i][j] = (int16_t)blade.a[i][j];
    }
    if (!bfm_forest_blade_transform(axis, &mount, eye, &glow)) return;
    Mat basis;
    for (unsigned i = 0; i < 3; ++i)
        for (unsigned j = 0; j < 3; ++j) basis.a[i][j] = glow.rotation[i][j];
    Mat facing = multiply(view, basis);
    Vec center = {{glow.translation[0], glow.translation[1], glow.translation[2]}};
    center = add(translation, transform(view, center));
    for (unsigned face = 0; face < 8 && a->used < MAX_DRAW; ++face) {
        int16_t corners[4][3];
        uint32_t colors[4];
        if (!bfm_forest_blade_quad(&a->fusion, face, tick, corners, colors)) return;
        Draw *d = &a->order[a->used];
        int visible = 1;
        for (unsigned i = 0; i < 4; ++i) {
            Vec local = {{corners[i][0], corners[i][1], corners[i][2]}};
            Vec point = add(center, transform(facing, local));
            if (point.v[2] <= 0) visible = 0;
            d->point[i] = project(point);
            for (unsigned ch = 0; ch < 3; ++ch)
                d->shade[i].v[ch] = (colors[i] >> (8 * ch)) & 255;
        }
        if (!visible) continue;
        d->face = &blade_light;
        d->depth = d->point[3].v[2] / 4;
        d->sequence = face;
        ++a->used;
    }
}

static const uint8_t *summoned_root(const BfmStairActors *a, unsigned field)
{
    unsigned path = 0;
    while (path + 1 < HERO_PATHS && field >= hero_paths[path + 1].field) ++path;
    return journey_pose(a->hero_roots[path], (field - hero_paths[path].field) / 2);
}

static void prepare_summoned_hero(BfmStairActors *a, unsigned field, Mat view, Vec translation, const int eye[3])
{
    if (field >= HERO_COURT_EXIT) return;
    Vec root = {{5,-658,-21}};
    int heading = 0;
    if (field >= HERO_LEAP) {
        const uint8_t *step = summoned_root(a, field);
        root = triple(step);
        heading = signed_word(step + 6);
    }
    Vec base = add(translation, transform(view, root));
    /* During Fillet's close-up Musashi's limbs reach past the near plane.
     * The shot omits both his body and its ground shadow. */
    if (field >= HERO_FILLET_CLOSE && field < HERO_COURT_WIDE && base.v[2] <= 128) return;
    if (field >= HERO_LEAP) shoe_shadow(a, root, heading, 2, view, translation, 0);
    const uint8_t *frame = summoned_pose(a, field);
    a->hero_expression = field < HERO_LEAP ? HERO_SLEEPING_FACE : HERO_STARTLED_FACE;
    if (field >= HERO_SHEATHE_START) a->hero_expression = HERO_NEUTRAL_FACE;
    Mat yaw = rotation(1, heading), turned = multiply(view, yaw);
    /* The first sixteen sheathing keys still carry the exposed blade. At
     * the mounted pose-bank change the scabbard and fittings stay visible. */
    uint32_t visible = UINT32_C(0x781fffff);
    if (frame == a->hero_frames[HERO_PRESENT_FUSION][0] || field >= HERO_SHEATHE_START) {
        visible |= (UINT32_C(1) << 21) | (UINT32_C(1) << 23) | (UINT32_C(1) << 26);
        if (field < HERO_SHEATHE_START + 2 * 16) visible |= UINT32_C(1) << 22;
    }
    for (unsigned j = 0; j < MUSASHI_FOREST_PLAYER_PARTS; ++j) {
        if (!(visible & (UINT32_C(1) << j))) continue;
        const uint8_t *bone = frame + 12 * j;
        Mat pose = angles(triple(bone + 6));
        Vec position = add(base, transform(turned, triple(bone)));
        Mat world = multiply(yaw, pose);
        prepare_model(a, a->hero + j, multiply(turned, pose), position, world);
        if (j == 22) {
            Vec grip = add(root, transform(yaw, triple(bone)));
            prepare_fusion_light(a, field / 2 + 1, eye, grip, world, view, translation);
        }
    }
}

static void prepare_departure_dust(BfmStairActors *a, unsigned field,
                                   const int camera[3][3], const int translation[3])
{
    enum { FIRST_BIRTH = 15904, LAST_BIRTH = 16004, PERIOD = 20 };
    if (field < FIRST_BIRTH || field >= LAST_BIRTH + PERIOD) return;
    unsigned elapsed = field - FIRST_BIRTH;
    unsigned age = (elapsed % PERIOD) / 2;
    if (!age) return;
    /* A puff stays at the exit-path position where the runner raised it. */
    unsigned birth = field - elapsed % PERIOD;
    Vec position = triple(summoned_root(a, birth));
    bfm_hero_smoke_project(&a->smoke, position.v, age, 2, camera, translation, queue_hero_smoke, a);
}

void bfm_stair_actors_prepare(BfmStairActors *a, unsigned field,
                              const int axes[3][3], const int origin[3], const int eye[3])
{
    if (!a) return;
    a->used = 0; a->next = 0;
    fillet_expression(a, field);
    unsigned profile = field >= 2758 ? 1 : 0;
    if (field >= 4634) profile = 2;
    a->selected_light = a->lights[profile];
    Mat view; memcpy(view.a, axes, sizeof(view.a));
    Vec translation = {{origin[0],origin[1],origin[2]}};
    masonry(a, field, view, translation);
    if (field >= 2760 && field < 4984) {
        unsigned turns = (1280u + ((field-2760u)/2u)*8u + bfm_ritual_extra_yaw(field)) & 4095u;
        Mat spin = rotation(1, (int)turns);
        prepare_model(a, &a->crystal, multiply(view, spin), translation, spin);
    }
    if (field >= 4984) prepare_summoned_hero(a, field, view, translation, eye);
    if (field >= 1510) {
        unsigned tick = (field - 1510) / 2;
        if (tick) for (unsigned i = 0; i < ACTORS; ++i) {
            const Performer *p = a->actor + i;
            const uint8_t *root, *frame;
            if (field >= 2756) {
                if (!crystal_keys(a, i, field, &root, &frame)) continue;
            } else {
                if (i >= 3) continue;
                actor_keys(a, i, tick, &root, &frame);
            }
            int heading = signed_word(root + 6);
            /* In the close shot of Musashi facing Shanky, the near plane
             * clips Shanky's floor quad while part of his chain remains visible. */
            int shadow_near = i == 4 && field >= 11304 && field < 11800 ? 128 : 0;
            shoe_shadow(a, triple(root), heading, 3, view, translation, shadow_near);
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
    bfm_ritual_light_prepare(&a->lights_on_crystal, field, axes, origin, queue_ritual_light, a);
    bfm_hero_smoke_prepare(&a->smoke, field, axes, origin, queue_hero_smoke, a);
    prepare_departure_dust(a, field, axes, origin);
    qsort(a->order, a->used, sizeof(*a->order), farthest);
}
