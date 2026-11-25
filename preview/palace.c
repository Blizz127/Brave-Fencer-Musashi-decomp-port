/* SPDX-License-Identifier: MIT */
#include "palace.h"
#include "stair_actors.h"
#include "palace_dialogue.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

enum { SCREEN_W = 320, TOP = 10, BOTTOM = 230, SECTOR = 2048 };
enum { MAX_GROUPS = 256, MAX_VERTICES = 4096, MAX_FACES = 4096, MAX_SCENE = 1 << 20 };

typedef struct { int x, y, z; } Point;
typedef struct { uint16_t u, v; } Texel;
typedef struct { Point eye, aim; } Pose;
typedef struct { Point right, down, forward; } Basis;
enum { INWARD_POSES = 100, INTERIOR_POSES = 224 };
typedef struct {
    Texel uv[4];
    uint16_t vertices[4], page_x, page_y, palette_x, palette_y;
    uint8_t count, depth_bias;
} Face;
typedef struct { int depth; size_t index; } Draw;

struct Palace {
    size_t vertex_count, face_count;
    Pose track[INTERIOR_POSES];
    BfmStairActors *actors;
    PalaceDialogue *dialogue;
    uint8_t *scene;
    unsigned member;
    Point vertices[MAX_VERTICES], projected[MAX_VERTICES];
    Face faces[MAX_FACES];
    Draw visible[MAX_FACES];
};

static uint16_t le16(const uint8_t *p)
{
    return (uint16_t)(p[0] | (unsigned)p[1] << 8);
}

static uint32_t le32(const uint8_t *p)
{
    return le16(p) | (uint32_t)le16(p + 2) << 16;
}

static int signed16(const uint8_t *p)
{
    uint16_t value = le16(p);
    return value < 32768u ? (int)value : (int)value - 65536;
}

/* Back-references are positions in a 1 KiB sliding dictionary, including
 * positions written earlier in the same token. */
static size_t expand(const uint8_t *src, size_t bytes, uint8_t *out)
{
    uint8_t ring[1024] = {0}, valid[1024] = {0};
    unsigned cursor = 1;
    size_t in = 0, size = 0;
    while (in < bytes) {
        unsigned flags = src[in++];
        for (unsigned bit = 1; bit <= 128; bit *= 2) {
            unsigned back = 0, count = 1;
            int literal = (flags & bit) != 0;
            if (literal) {
                if (in == bytes) return 0;
            } else {
                if (bytes - in < 2) return 0;
                unsigned token = le16(src + in);
                in += 2;
                back = token & 1023u;
                if (back == 0) return size;
                count = (token >> 10) + 2;
            }
            if (count > MAX_SCENE - size) return 0;
            for (unsigned i = 0; i < count; ++i) {
                if (!literal && !valid[back]) return 0;
                uint8_t value = literal ? src[in++] : ring[back];
                out[size++] = value;
                ring[cursor] = value;
                valid[cursor] = 1;
                cursor = (cursor + 1) & 1023u;
                back = (back + 1) & 1023u;
            }
        }
    }
    return 0;
}

static size_t read_scene(const MusashiDiscMedia *disc, unsigned member, uint8_t *scene)
{
    uint8_t header[24];
    size_t size;
    if (!musashi_disc_media_read_root_range(disc, "SC01.CD", 0, header, sizeof(header), &size))
        return 0;
    if (member > 1 || member >= le32(header)) return 0;
    uint32_t start = le32(header + 8 + 8 * member);
    uint32_t member_bytes = le32(header + 12 + 8 * member);
    if (!start || start > size / SECTOR) return 0;
    size_t offset = (size_t)start * SECTOR;
    if (member_bytes > size - offset || member_bytes < SECTOR ||
        !musashi_disc_media_read_root_range(disc, "SC01.CD", offset, header, sizeof(header), &size) ||
        memcmp(header, "PAC", 4) || le32(header + 4) != 4)
        return 0;
    uint32_t span = le32(header + 12);
    if (span <= SECTOR || span > member_bytes || span - SECTOR > MAX_SCENE) return 0;
    size_t bytes = span - SECTOR;
    uint8_t *packed = malloc(bytes);
    size_t result = 0;
    if (packed && musashi_disc_media_read_root_range(disc, "SC01.CD", offset + SECTOR,
                                                     packed, bytes, &size))
        result = expand(packed, bytes, scene);
    free(packed);
    return result;
}

static int take_face(Face *face, const uint8_t *entry, const uint8_t *material,
                     size_t vertex_count, unsigned corners)
{
    unsigned palette = le16(material + 6), page = le16(material + 10);
    if (material[0] != 128 || material[1] != 128 || material[2] != 128 ||
        material[3] != entry[3] || ((page >> 7) & 3) != 1)
        return 0;
    face->page_x = (uint16_t)((page & 15) * 64);
    face->page_y = (uint16_t)((page & 16) * 16);
    face->palette_x = (uint16_t)((palette & 63) * 16);
    face->palette_y = (uint16_t)(palette >> 6);
    if (face->palette_x > 1024 - 256 ||
        face->palette_y >= 512) return 0;
    face->count = (uint8_t)corners;
    face->depth_bias = (uint8_t)(le16(entry + 10) & 1u);
    for (unsigned j = 0; j < corners; ++j) {
        unsigned vertex_ref = le16(entry + 4 + j * 2);
        /* The final stored slot also carries primitive flags. */
        if (j < 3 && (vertex_ref & 7u)) return 0;
        unsigned vertex = vertex_ref / 8;
        if (vertex >= vertex_count) return 0;
        face->vertices[j] = (uint16_t)vertex;
        unsigned uv_at = j < 2 ? 4 + j * 4 : 12 + (j - 2) * 2;
        face->uv[j] = (Texel){material[uv_at], material[uv_at + 1]};
    }
    return 1;
}

/* The US opening scene's palace directory contains relative offsets. Each
 * spatial group owns a contiguous list of material/vertex references. */
static int load_mesh(Palace *palace, const uint8_t *scene, size_t size)
{
    const size_t directory = palace->member ? 0x61e20 : 0x5a6e8;
    if (size < directory + 12) return 0;
    const uint8_t *root = scene + directory + 12;
    size_t available = size - directory - 12;
    unsigned groups = le32(root - 12), materials = le32(root - 8), vertices = le32(root - 4);
    if (!groups || groups > MAX_GROUPS || materials != 20 * groups ||
        vertices < materials || vertices > available || (vertices - materials) % 16)
        return 0;
    unsigned faces = le32(root + 16);
    if (faces < vertices || faces > available || (faces - vertices) % 8) return 0;
    palace->vertex_count = (faces - vertices) / 8;
    if (!palace->vertex_count || palace->vertex_count > MAX_VERTICES) return 0;
    for (size_t i = 0; i < palace->vertex_count; ++i) {
        const uint8_t *v = root + vertices + 8 * i;
        palace->vertices[i] = (Point){signed16(v), signed16(v + 2), signed16(v + 4)};
    }
    size_t next = faces;
    for (unsigned g = 0; g < groups; ++g) {
        const uint8_t *group = root + g * 20;
        uint32_t count = le32(group + 12);
        if (le32(group + 16) != next || count > (available - next) / 12) return 0;
        for (size_t i = 0; i < count; ++i, next += 12) {
            const uint8_t *entry = root + next;
            if (entry[3] == 0x28) continue; /* Interior flat quad outside this shot. */
            if (entry[3] != 0x24 && entry[3] != 0x2c) return 0;
            unsigned offset = le16(entry);
            if (offset % 16 || offset >= vertices - materials || palace->face_count == MAX_FACES)
                return 0;
            if (!take_face(&palace->faces[palace->face_count], entry, root + materials + offset,
                           palace->vertex_count, entry[3] == 0x24 ? 3 : 4))
                return 0;
            ++palace->face_count;
        }
    }
    return palace->face_count != 0;
}

/* Each authored camera record holds signed XYZ eye and aim positions, then
 * a roll value. These two opening shots use an unrolled camera. */
static int read_track(Palace *palace, const uint8_t *scene, size_t bytes)
{
    size_t begin = palace->member ? 0x9b424 : 0x7a618;
    unsigned count = palace->member ? INTERIOR_POSES : INWARD_POSES;
    if (begin > bytes || count > (bytes - begin) / 14) return 0;
    for (unsigned i = 0; i < count; ++i) {
        const uint8_t *p = scene + begin + 14 * i;
        Pose pose = {{signed16(p), signed16(p + 2), signed16(p + 4)},
                     {signed16(p + 6), signed16(p + 8), signed16(p + 10)}};
        if (signed16(p + 12) || (pose.eye.x == pose.aim.x && pose.eye.z == pose.aim.z))
            return 0;
        palace->track[i] = pose;
    }
    return 1;
}

Palace *bfm_palace_load(const MusashiDiscMedia *disc, unsigned member,
                        const uint16_t tiles[512][1024])
{
    uint8_t *scene = malloc(MAX_SCENE);
    Palace *palace = calloc(1, sizeof(*palace));
    size_t bytes = scene && palace ? read_scene(disc, member, scene) : 0;
    if (palace) palace->member = member;
    if (!bytes || !load_mesh(palace, scene, bytes) || !read_track(palace, scene, bytes)) {
        free(palace);
        palace = NULL;
    }
    if (palace && member) {
        palace->actors = bfm_stair_actors_load(scene, bytes);
        palace->dialogue = bfm_palace_dialogue_load(disc, scene, bytes, tiles);
        if (!palace->actors || !palace->dialogue) {
            bfm_stair_actors_free(palace->actors);
            bfm_palace_dialogue_free(palace->dialogue);
            free(palace);
            palace = NULL;
        } else {
            palace->scene = scene;
            scene = NULL;
        }
    }
    free(scene);
    return palace;
}

void bfm_palace_free(Palace *palace)
{
    if (palace) {
        bfm_palace_dialogue_free(palace->dialogue);
        bfm_stair_actors_free(palace->actors);
        free(palace->scene);
    }
    free(palace);
}

static int floor_div(int64_t n, int d)
{
    return n >= 0 ? (int)(n / d) : (int)(-((-n + d - 1) / d));
}

static int smaller(int a, int b) { return a < b ? a : b; }
static int larger(int a, int b) { return a > b ? a : b; }

static int camera_axis(Point v, int a, int b, int c)
{
    return floor_div((int64_t)v.x * a + (int64_t)v.y * b + (int64_t)v.z * c, 4096);
}

static int projection_coord(int coordinate, int reciprocal, int center)
{
    int screen = floor_div((int64_t)coordinate * reciprocal, 65536);
    return center + smaller(1023, larger(-1024, screen));
}

static uint64_t squared(int v)
{
    return (uint64_t)((int64_t)v * v);
}

static unsigned integer_length(uint64_t squared_length)
{
    uint64_t remainder = squared_length, root = 0, bit = UINT64_C(1) << 62;
    while (bit > remainder) bit >>= 2;
    while (bit) {
        if (remainder >= root + bit) {
            remainder -= root + bit;
            root = (root >> 1) + bit;
        } else {
            root >>= 1;
        }
        bit >>= 2;
    }
    return (unsigned)root;
}

/* The camera handles short axes with an integer-length denominator. Its
 * mantissa advances two bits at a time, as the length is a square root. */
static int axis_magnitude(uint64_t component_squared, uint64_t total, int component)
{
    if (component_squared < UINT64_C(1) << 19) {
        unsigned high_bit = 0;
        for (uint64_t x = total; x >>= 1;) ++high_bit;
        unsigned shift = high_bit > 6 ? (high_bit - 6) / 2 * 2 : 0;
        uint64_t shortened = (total >> shift) << shift;
        int denominator = (int)integer_length(shortened);
        return (int)((int64_t)component * 4096 / denominator);
    }
    uint64_t ratio = (component_squared << 13) / total;
    int magnitude = (int)integer_length(ratio << 11);
    return component < 0 ? -magnitude : magnitude;
}

static Basis look_at(Pose pose)
{
    int dx = pose.aim.x - pose.eye.x, dy = pose.aim.y - pose.eye.y;
    int dz = pose.aim.z - pose.eye.z;
    uint64_t horizontal = squared(dx) + squared(dz);
    uint64_t total = horizontal + squared(dy);
    int sy = axis_magnitude(squared(dx), horizontal, -dx);
    int cy = axis_magnitude(squared(dz), horizontal, dz);
    int cp = axis_magnitude(horizontal, total, (int)integer_length(horizontal));
    int sp = axis_magnitude(squared(dy), total, dy);
    return (Basis){{cy, 0, sy}, {floor_div((int64_t)sy * sp, 4096), cp,
                                  floor_div(-(int64_t)cy * sp, 4096)},
                   {floor_div(-(int64_t)sy * cp, 4096), sp,
                    floor_div((int64_t)cy * cp, 4096)}};
}

static void project_mesh(Palace *palace, unsigned field)
{
    Basis axes;
    Point eye;
    int focal = palace->member ? 256 : 400;
    if (palace->member) {
        unsigned tick = field >= 1512 ? (field - 1512) / 2 : 0;
        Pose pose = palace->track[tick < INTERIOR_POSES ? tick : INTERIOR_POSES - 1];
        eye = pose.eye;
        /* The wide stairwell view changes yaw by one scene unit as the
         * group descends. An impact shakes the camera, then decays. */
        axes = pose.aim.x == palace->track[0].aim.x
                   ? (Basis){{-1130,0,3946},{388,4074,111},{-3925,403,-1124}}
                   : (Basis){{-1124,0,3947},{388,4096,110},{-3947,403,-1124}};
        if (field >= 1480 && field < 1544) {
            unsigned wave = (field - 1480) / 4;
            int amplitude = 8 - (int)(wave / 2);
            eye.y += wave % 2 ? amplitude : -amplitude;
        }
    } else if (field >= 958) {
        unsigned tick = (field - 952) / 2;
        Pose pose = palace->track[tick < INWARD_POSES ? tick : INWARD_POSES - 1];
        axes = look_at(pose);
        eye = pose.eye;
    } else {
        unsigned ticks_to_end = field < 828 ? (829 - field) / 2 : 0;
        int eye_travel = (int)((87 * ticks_to_end + 7) / 13);
        int aim_travel = (int)((87 * ticks_to_end + 6) / 13);
        int pitch = eye_travel - aim_travel;
        eye = (Point){582, -715 - eye_travel, -1064};
        axes = (Basis){{3106, 0, 2669}, {-531 + 2 * pitch, 4014 + pitch, 617 - 3 * pitch},
                       {-2616 - pitch, -814 + 3 * pitch, 3043 + pitch}};
    }
    Point reverse = {-eye.x, -eye.y, -eye.z};
    int tx = camera_axis(reverse, axes.right.x, axes.right.y, axes.right.z);
    int ty = camera_axis(reverse, axes.down.x, axes.down.y, axes.down.z);
    int tz = camera_axis(reverse, axes.forward.x, axes.forward.y, axes.forward.z);
    if (palace->actors) {
        const int matrix[3][3] = {{axes.right.x, axes.right.y, axes.right.z},
                                  {axes.down.x, axes.down.y, axes.down.z},
                                  {axes.forward.x, axes.forward.y, axes.forward.z}};
        const int position[3] = {tx, ty, tz};
        bfm_stair_actors_prepare(palace->actors, field, matrix, position);
    }
    for (size_t i = 0; i < palace->vertex_count; ++i) {
        Point v = palace->vertices[i];
        int x = camera_axis(v, axes.right.x, axes.right.y, axes.right.z) + tx;
        int y = camera_axis(v, axes.down.x, axes.down.y, axes.down.z) + ty;
        int z = camera_axis(v, axes.forward.x, axes.forward.y, axes.forward.z) + tz;
        int reciprocal = z > 0 ? smaller(131071, (focal * 65536 + z / 2) / z) : 131071;
        palace->projected[i] = (Point){projection_coord(x, reciprocal, 160),
                                       projection_coord(y, reciprocal, 120), z};
    }
}

static int edge(Point a, Point b, int x, int y)
{
    return (b.x - a.x) * (y - a.y) - (b.y - a.y) * (x - a.x);
}

static int closed_edge(Point a, Point b)
{
    return a.y > b.y || (a.y == b.y && a.x < b.x);
}

static int far_first(const void *left, const void *right)
{
    const Draw *a = left, *b = right;
    if (a->depth != b->depth) return a->depth > b->depth ? -1 : 1;
    return a->index > b->index ? -1 : a->index < b->index;
}

static uint32_t color(uint16_t ink)
{
    uint32_t r = (ink & 31u) * 255 / 31;
    uint32_t g = ((ink >> 5) & 31u) * 255 / 31;
    uint32_t blue = ((ink >> 10) & 31u) * 255 / 31;
    return UINT32_C(0xff000000) | (r << 16) | (g << 8) | blue;
}

static void triangle(const uint16_t tiles[512][1024], const Face *face,
                     const Point p[3], const Texel uv[3], uint32_t *pixels)
{
    Point a = p[0], b = p[1], c = p[2];
    int area = edge(a, b, c.x, c.y);
    if (area <= 0) return;
    int left = larger(0, smaller(a.x, smaller(b.x, c.x)));
    int right = smaller(SCREEN_W - 1, larger(a.x, larger(b.x, c.x)));
    int top = larger(TOP, smaller(a.y, smaller(b.y, c.y)));
    int bottom = smaller(BOTTOM - 1, larger(a.y, larger(b.y, c.y)));
    int include_a = closed_edge(b, c), include_b = closed_edge(c, a), include_c = closed_edge(a, b);
    for (int y = top; y <= bottom; ++y) {
        for (int x = left; x <= right; ++x) {
            int wa = edge(b, c, x, y), wb = edge(c, a, x, y), wc = area - wa - wb;
            if (wa < !include_a || wb < !include_b || wc < !include_c) continue;
            unsigned u = (unsigned)(((int64_t)wa * uv[0].u + (int64_t)wb * uv[1].u +
                                     (int64_t)wc * uv[2].u + area / 2) / area);
            unsigned v = (unsigned)(((int64_t)wa * uv[0].v + (int64_t)wb * uv[1].v +
                                     (int64_t)wc * uv[2].v + area / 2) / area);
            uint16_t pair = tiles[face->page_y + v][(face->page_x + u / 2) & 1023u];
            unsigned index = (pair >> (8 * (u & 1))) & 255u;
            uint16_t ink = tiles[face->palette_y][face->palette_x + index];
            if (ink)
                pixels[y * SCREEN_W + x] = color(ink);
        }
    }
}

/* A location caption appears after the camera settles. Its single 4-bit
 * sprite and palette come from the same scene texture upload as the sky. */
static void place_name(const uint16_t tiles[512][1024], uint32_t *pixels)
{
    for (unsigned y = 0; y < 16; ++y) {
        for (unsigned x = 0; x < 128; ++x) {
            uint16_t packed = tiles[384 + y][896 + x / 4];
            unsigned index = (packed >> (4 * (x & 3))) & 15u;
            uint16_t ink = tiles[259][368 + index];
            if (ink)
                pixels[(200 + y) * SCREEN_W + 96 + x] = color(ink);
        }
    }
}

void bfm_palace_draw(Palace *palace, const uint16_t tiles[512][1024],
                     unsigned field, uint32_t *pixels)
{
    if (field < 696) return;
    project_mesh(palace, field);
    size_t drawn = 0;
    for (size_t i = 0; i < palace->face_count; ++i) {
        const Face *face = &palace->faces[i];
        Point p[4];
        int near_z = INT_MAX, far_z = 0;
        for (unsigned j = 0; j < face->count; ++j) {
            p[j] = palace->projected[face->vertices[j]];
            near_z = smaller(near_z, p[j].z);
            far_z = larger(far_z, p[j].z);
        }
        if (near_z <= (palace->member ? 0 : 200) || edge(p[0], p[1], p[2].x, p[2].y) <= 0) continue;
        /* Furthest-vertex depth determines the four-unit ordering bucket.
         * Certain surfaces have an additional 512-unit draw priority. */
        palace->visible[drawn++] = (Draw){(far_z + 512 * face->depth_bias) / 4, i};
    }
    qsort(palace->visible, drawn, sizeof(*palace->visible), far_first);
    for (size_t i = 0; i < drawn; ++i) {
        bfm_stair_actors_draw_to(palace->actors, palace->visible[i].depth, tiles, pixels);
        const Face *face = &palace->faces[palace->visible[i].index];
        Point p[4];
        for (unsigned j = 0; j < face->count; ++j)
            p[j] = palace->projected[face->vertices[j]];
        triangle(tiles, face, p, face->uv, pixels);
        if (face->count == 4) {
            Point back[3] = {p[2], p[1], p[3]};
            Texel uv[3] = {face->uv[2], face->uv[1], face->uv[3]};
            triangle(tiles, face, back, uv, pixels);
        }
    }
    bfm_stair_actors_draw_to(palace->actors, INT_MIN, tiles, pixels);
    if (palace->dialogue) bfm_palace_dialogue_draw(palace->dialogue, field, (uint32_t (*)[320])pixels);
    if (field >= 832 && field < 952)
        place_name(tiles, pixels);
}
