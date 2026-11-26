/* SPDX-License-Identifier: MIT */
#include "palace.h"
#include "bedroom_field.h"
#include "stair_actors.h"
#include "palace_dialogue.h"
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { SCREEN_W = 320, TOP = 10, BOTTOM = 230, SECTOR = 2048 };
enum { MAX_VERTICES = 4096, MAX_FACES = 4096, MAX_SCENE = BFM_FIELD_SCENE_CAPACITY };

typedef struct { int x, y, z; } Point;
typedef struct { uint16_t u, v; } Texel;
typedef struct { Point eye, aim; } Pose;
typedef struct { Point right, down, forward; } Basis;
enum { INWARD_POSES = 100, STAIR_POSES = 504, LANDING_POSES = 98,
       CRYSTAL_WIDE_POSES = 50, STILL_POSES = 2, GATHERED_POSES = 70,
       MUSASHI_ORBIT_POSES = 268, JUMP_CAMERA_POSES = 60, FILLET_REPLY_POSES = 140,
       FUSION_OFFER_POSES = 320, NOTE_CAMERA_POSES = 90, TOWER_ROUTE_POSES = 375,
       FAREWELL_POSES = 145, COURTIERS_POSES = 204, FILLET_TURN_POSES = 13, DOOR_POSES = 223 };
static const struct { unsigned start, count; size_t offset; } court_shots[] = {
    {1512, STAIR_POSES, 0x9b424}, {2520, LANDING_POSES, 0x9d504},
    {2760, CRYSTAL_WIDE_POSES, 0x9da70}, {2854, STILL_POSES, 0x9dd3c},
    {3172, GATHERED_POSES, 0x9dd68},
    /* Frontal cut, Fillet overhead, then opposite sides of the ritual circle. */
    {4012, STILL_POSES, 0xa46e0}, {4016, STILL_POSES, 0x9e14c},
    {4148, STILL_POSES, 0xa470c}, {4212, STILL_POSES, 0xa4738},
    {4276, STILL_POSES, 0xa4764},   /* Fillet faces the crystal. */
    {4636, STILL_POSES, 0xa4790},   /* Butler Livers reacts to the flash. */
    {4984, STILL_POSES, 0xa47bc},   /* The court sees Musashi on the dais. */
    {5156, MUSASHI_ORBIT_POSES, 0xa47e8},
    {5698, STILL_POSES, 0xa56a0},   /* Fillet addresses the court below the dais. */
    {6676, STILL_POSES, 0xa56cc},   /* Musashi stirs under the palace light. */
    {6874, JUMP_CAMERA_POSES, 0xa56f8},
    {7082, STILL_POSES, 0xa5a50},   /* Musashi turns away from the great door. */
    {7406, FILLET_REPLY_POSES, 0xa5a7c}, /* The view moves closer during Fillet’s reply. */
    {7774, STILL_POSES, 0xa6234},   /* The court intercedes, seen from the side. */
    {8850, STILL_POSES, 0xa62b8},   /* An attendant objects to the Steward’s request. */
    {9196, STILL_POSES, 0xa62e4},   /* The Steward answers in the wide room. */
    {9590, STILL_POSES, 0xa6310},   /* Musashi asks to go home. */
    {10032, STILL_POSES, 0xa633c},  /* Shanky crosses the chamber. */
    {11304, STILL_POSES, 0xa6368},  /* Musashi faces the Scribe. */
    {11798, STILL_POSES, 0xa6394},  /* The court comes back into view. */
    {11922, STILL_POSES, 0xa63c0},  /* The Steward sends Musashi to Spiral Tower. */
    {12334, FUSION_OFFER_POSES, 0xa63ec}, /* Butler Livers approaches with Fusion. */
    {12934, STILL_POSES, 0xa757c},  /* Musashi raises Fusion. */
    {13118, STILL_POSES, 0xa75a8},  /* The courtiers explain the sword. */
    {14104, STILL_POSES, 0xa75d4},
    {14302, STILL_POSES, 0xa7600},  /* Musashi answers and lowers Fusion. */
    {14704, STILL_POSES, 0xa762c},  /* Musashi wears the sword at his side. */
    {14950, NOTE_CAMERA_POSES, 0xa7658},
    {15104, TOWER_ROUTE_POSES, 0xa7b54}, /* Fillet explains the way to the tower. */
    {15820, FAREWELL_POSES, 0xa8fe4},    /* Musashi runs out of the chamber. */
    {16070, COURTIERS_POSES, 0xa97e0},
    {16426, FILLET_TURN_POSES, 0xaa318},
    {16814, DOOR_POSES, 0xaa3dc}          /* The remaining court watches the exit. */
};
enum { COURT_SHOT_COUNT = sizeof(court_shots) / sizeof(court_shots[0]) };
typedef struct {
    Texel uv[4];
    uint16_t vertices[4], page_x, page_y, palette_x, palette_y;
    uint8_t count, depth_bias, texture_bits, blend_mode, semi_transparent, rgb[3];
} Face;
typedef struct { int depth; size_t index; } Draw;

struct Palace {
    size_t vertex_count, face_count;
    Pose track[STAIR_POSES + LANDING_POSES + CRYSTAL_WIDE_POSES + GATHERED_POSES +
               MUSASHI_ORBIT_POSES + JUMP_CAMERA_POSES + FILLET_REPLY_POSES +
               FUSION_OFFER_POSES + NOTE_CAMERA_POSES + TOWER_ROUTE_POSES + FAREWELL_POSES +
               COURTIERS_POSES + FILLET_TURN_POSES + DOOR_POSES + 24 * STILL_POSES];
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

size_t bfm_read_archive_field_scene(const MusashiDiscMedia *disc, const char *archive,
                                    unsigned member, uint8_t scene[BFM_FIELD_SCENE_CAPACITY])
{
    uint8_t header[SECTOR];
    size_t size;
    if (!disc || !archive || !scene || !musashi_disc_media_read_root_range(disc, archive, 0,
                                                               header, sizeof(header), &size))
        return 0;
    if (le32(header) > (SECTOR - 8) / 8 || member >= le32(header)) return 0;
    uint32_t start = le32(header + 8 + 8 * member);
    uint32_t member_bytes = le32(header + 12 + 8 * member);
    if (!start || start > size / SECTOR) return 0;
    size_t offset = (size_t)start * SECTOR;
    if (member_bytes > size - offset || member_bytes < SECTOR ||
        !musashi_disc_media_read_root_range(disc, archive, offset, header, sizeof(header), &size) ||
        memcmp(header, "PAC", 4) || le32(header + 4) != 4)
        return 0;
    uint32_t span = le32(header + 12);
    if (span <= SECTOR || span > member_bytes || span - SECTOR > MAX_SCENE) return 0;
    size_t bytes = span - SECTOR;
    uint8_t *packed = malloc(bytes);
    size_t result = 0;
    if (packed && musashi_disc_media_read_root_range(disc, archive, offset + SECTOR,
                                                     packed, bytes, &size))
        result = expand(packed, bytes, scene);
    free(packed);
    return result;
}

size_t bfm_read_field_scene(const MusashiDiscMedia *disc, unsigned member,
                            uint8_t scene[BFM_FIELD_SCENE_CAPACITY])
{
    return bfm_read_archive_field_scene(disc, "SC01.CD", member, scene);
}

static int take_face(Face *face, const uint8_t *entry, const uint8_t *material,
                     size_t vertex_count, unsigned corners)
{
    if (material) {
        unsigned palette = le16(material + 6), page = le16(material + 10);
        unsigned depth = (page >> 7) & 3;
        if (material[3] != entry[3] || depth > 1) return 0;
        face->texture_bits = depth ? 8 : 4;
        face->blend_mode = (uint8_t)((page >> 5) & 3u);
        face->semi_transparent = (entry[3] & 2u) != 0;
        face->page_x = (uint16_t)((page & 15) * 64);
        face->page_y = (uint16_t)((page & 16) * 16);
        face->palette_x = (uint16_t)((palette & 63) * 16);
        face->palette_y = (uint16_t)(palette >> 6);
        if (face->palette_x > 1024 - (1u << face->texture_bits) ||
            face->palette_y >= 512) return 0;
    } else {
        face->texture_bits = 0;
    }
    memcpy(face->rgb, material ? material : entry, sizeof(face->rgb));
    face->count = (uint8_t)corners;
    face->depth_bias = (uint8_t)(le16(entry + 10) & 1u);
    for (unsigned j = 0; j < corners; ++j) {
        unsigned vertex_ref = le16(entry + 4 + j * 2);
        /* The final stored slot also carries primitive flags. */
        if (j < 3 && (vertex_ref & 7u)) return 0;
        unsigned vertex = vertex_ref / 8;
        if (vertex >= vertex_count) return 0;
        face->vertices[j] = (uint16_t)vertex;
        if (material) {
            unsigned uv_at = j < 2 ? 4 + j * 4 : 12 + (j - 2) * 2;
            face->uv[j] = (Texel){material[uv_at], material[uv_at + 1]};
        }
    }
    return 1;
}

/* The US opening scene's palace directory contains relative offsets. Each
 * spatial group owns a contiguous list of material/vertex references. */
static int load_mesh(Palace *palace, const uint8_t *scene, size_t size, size_t directory)
{
    if (directory > size || size - directory < 12) return 0;
    const uint8_t *root = scene + directory + 12;
    size_t available = size - directory - 12;
    unsigned groups = le32(root - 12), materials = le32(root - 8), vertices = le32(root - 4);
    if (!groups || groups > available / 20 || materials != (size_t)20 * groups ||
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
            unsigned kind = entry[3];
            const uint8_t *material = NULL;
            if (kind & 4) {
                unsigned offset = le16(entry);
                /* Flat textured triangles and quads can request blending. */
                if ((kind & ~0x0au) != 0x24 || offset % 16 || offset >= vertices - materials)
                    return 0;
                material = root + materials + offset;
            } else if (kind != 0x20 && kind != 0x28) {
                return 0;
            }
            if (palace->face_count == MAX_FACES ||
                !take_face(&palace->faces[palace->face_count], entry, material,
                           palace->vertex_count, (kind & 8) ? 4 : 3))
                return 0;
            ++palace->face_count;
        }
    }
    return palace->face_count != 0;
}

/* Each mounted shot holds signed XYZ eye and aim positions followed by roll.
 * All shots supported here use an unrolled camera. */
static int read_poses(Pose *out, const uint8_t *scene, size_t bytes, size_t begin, unsigned count)
{
    if (begin > bytes || count > (bytes - begin) / 14) return 0;
    for (unsigned i = 0; i < count; ++i) {
        const uint8_t *p = scene + begin + 14*i;
        Pose pose = {{signed16(p), signed16(p + 2), signed16(p + 4)},
                     {signed16(p + 6), signed16(p + 8), signed16(p + 10)}};
        if (signed16(p + 12) || (pose.eye.x == pose.aim.x && pose.eye.z == pose.aim.z))
            return 0;
        out[i] = pose;
    }
    return 1;
}

static int read_track(Palace *palace, const uint8_t *scene, size_t bytes)
{
    if (!palace->member) return read_poses(palace->track, scene, bytes, 0x7a618, INWARD_POSES);
    size_t first = 0;
    for (unsigned i = 0; i < COURT_SHOT_COUNT; ++i) {
        unsigned count = court_shots[i].count;
        if (count > sizeof(palace->track) / sizeof(*palace->track) - first ||
            !read_poses(palace->track + first, scene, bytes, court_shots[i].offset, count)) return 0;
        first += count;
    }
    return 1;
}

Palace *bfm_palace_load(const MusashiDiscMedia *disc, unsigned member,
                        const uint16_t tiles[512][1024])
{
    uint8_t *scene = malloc(MAX_SCENE);
    Palace *palace = calloc(1, sizeof(*palace));
    size_t bytes = scene && palace && member <= 1 ? bfm_read_field_scene(disc, member, scene) : 0;
    if (palace) palace->member = member;
    if (!bytes || !load_mesh(palace, scene, bytes, member ? 0x61e20 : 0x5a6e8) || !read_track(palace, scene, bytes)) {
        free(palace);
        palace = NULL;
    }
    if (palace && member) {
        palace->actors = bfm_stair_actors_load(disc, scene, bytes);
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

PalaceDialogue *bfm_palace_speech(Palace *palace)
{
    return palace ? palace->dialogue : NULL;
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
static unsigned camera_length(uint64_t squared_length)
{
    unsigned high_bit = 0;
    for (uint64_t bits = squared_length; bits >>= 1;) ++high_bit;
    unsigned shift = high_bit > 6 ? (high_bit - 6) / 2 * 2 : 0;
    return integer_length((squared_length >> shift) << shift);
}

static int axis_magnitude(uint64_t component_squared, uint64_t total, int component)
{
    if (component_squared < UINT64_C(1) << 19) {
        int denominator = (int)camera_length(total);
        return (int)((int64_t)component * 4096 / denominator);
    }
    uint64_t ratio = (component_squared << 13) / total;
    int magnitude = (int)integer_length(ratio << 11);
    return component < 0 ? -magnitude : magnitude;
}

static Basis look_at(Pose pose, int use_scene_length)
{
    int dx = pose.aim.x - pose.eye.x, dy = pose.aim.y - pose.eye.y;
    int dz = pose.aim.z - pose.eye.z;
    uint64_t horizontal = squared(dx) + squared(dz);
    uint64_t total = horizontal + squared(dy);
    int sy = axis_magnitude(squared(dx), horizontal, -dx);
    int cy = axis_magnitude(squared(dz), horizontal, dz);
    int length = (int)(use_scene_length ? camera_length(horizontal) : integer_length(horizontal));
    int cp = use_scene_length && horizontal >= (UINT64_C(1) << 19)
                 ? (int)integer_length((horizontal << 24) / total)
                 : axis_magnitude(horizontal, total, length);
    int sp = axis_magnitude(squared(dy), total, dy);
    return (Basis){{cy, 0, sy}, {floor_div((int64_t)sy * sp, 4096), cp,
                                  floor_div(-(int64_t)cy * sp, 4096)},
                   {floor_div(-(int64_t)sy * cp, 4096), sp,
                    floor_div((int64_t)cy * cp, 4096)}};
}

static BfmBedroomView make_view(Point eye, Basis axes, int focal)
{
    Point reverse = {-eye.x, -eye.y, -eye.z};
    int tx = camera_axis(reverse, axes.right.x, axes.right.y, axes.right.z);
    int ty = camera_axis(reverse, axes.down.x, axes.down.y, axes.down.z);
    int tz = camera_axis(reverse, axes.forward.x, axes.forward.y, axes.forward.z);
    BfmBedroomView view = {
        .axes = {{axes.right.x, axes.right.y, axes.right.z},
                 {axes.down.x, axes.down.y, axes.down.z},
                 {axes.forward.x, axes.forward.y, axes.forward.z}},
        .translation = {tx, ty, tz}, .focal = focal,
        .eye = {eye.x, eye.y, eye.z}
    };
    return view;
}

static Point project_point(const BfmBedroomView *view, Point v)
{
    const int (*axis)[3] = view->axes;
    int x = camera_axis(v, axis[0][0], axis[0][1], axis[0][2]) + view->translation[0];
    int y = camera_axis(v, axis[1][0], axis[1][1], axis[1][2]) + view->translation[1];
    int z = camera_axis(v, axis[2][0], axis[2][1], axis[2][2]) + view->translation[2];
    int reciprocal = z > 0 ? smaller(131071, (view->focal * 65536 + z / 2) / z) : 131071;
    return (Point){projection_coord(x, reciprocal, 160),
                   projection_coord(y, reciprocal, 120), z};
}

static void project_vertices(Palace *palace, const BfmBedroomView *view)
{
    for (size_t i = 0; i < palace->vertex_count; ++i)
        palace->projected[i] = project_point(view, palace->vertices[i]);
}

static void project_mesh(Palace *palace, unsigned field)
{
    Basis axes;
    Point eye;
    int focal = palace->member ? 256 : 400;
    if (palace->member) {
        unsigned shot = 0, first = 0;
        while (shot + 1 < COURT_SHOT_COUNT && field >= court_shots[shot + 1].start) {
            first += court_shots[shot].count;
            ++shot;
        }
        unsigned start = court_shots[shot].start, count = court_shots[shot].count;
        unsigned since = field > start ? (field - start) / 2 : 0;
        unsigned tick = first + (since < count ? since : count - 1);
        Pose pose = palace->track[tick];
        eye = pose.eye;
        axes = look_at(pose, 1);
        /* The court shakes again before the Steward sends Musashi to the tower. */
        unsigned shake_start = field >= 11838 ? 11838 : field >= 4570 ? 4570 : 1480;
        if (field >= shake_start && field - shake_start < 64) {
            unsigned wave = (field - shake_start) / 4;
            int amplitude = 8 - (int)(wave / 2);
            eye.y += wave % 2 ? amplitude : -amplitude;
        }
    } else if (field >= 958) {
        unsigned tick = (field - 952) / 2;
        Pose pose = palace->track[tick < INWARD_POSES ? tick : INWARD_POSES - 1];
        axes = look_at(pose, 0);
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
    BfmBedroomView view = make_view(eye, axes, focal);
    if (palace->actors)
        bfm_stair_actors_prepare(palace->actors, field, view.axes, view.translation, view.eye);
    project_vertices(palace, &view);
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

/* Blend in five-bit framebuffer space. The texture palette supplies the
 * mask bit independently of the polygon's blend request and texture page. */
static uint16_t blend_texel(uint16_t source, uint32_t background, unsigned mode)
{
    unsigned combined = 0;
    for (unsigned channel = 0; channel < 3; ++channel) {
        unsigned front = (source >> (channel * 5)) & 31;
        unsigned back = (background >> (19 - channel * 8)) & 31;
        unsigned level;
        switch (mode) {
        case 0: level = (back + front) / 2; break;
        case 1: level = back + front; break;
        case 2: level = back > front ? back - front : 0; break;
        default: level = back + front / 4; break;
        }
        combined |= (level > 31 ? 31 : level) << (channel * 5);
    }
    return (uint16_t)combined;
}

/* The ritual darkens these mounted floor and masonry palettes. The ceiling
 * light, actors and hovering crystal keep their own colors. */
static const unsigned ritual_rows[] = {480, 482, 483, 484, 485, 486, 487, 488};
enum { RITUAL_PALETTES = sizeof(ritual_rows) / sizeof(ritual_rows[0]) };

static int ritual_channel(int original, int dark, int dim_steps, int restore_steps)
{
    if (restore_steps)
        return smaller(larger(0, original - 1), dark + restore_steps);
    return larger(original - dim_steps, dark);
}

static void ritual_colors(const uint16_t tiles[512][1024], int dim_steps, int restore_steps,
                          uint16_t palettes[RITUAL_PALETTES][256])
{
    for (unsigned row = 0; row < RITUAL_PALETTES; ++row) {
        for (unsigned i = 0; i < 256; ++i) {
            uint16_t ink = tiles[ritual_rows[row]][i];
            int r = ink & 31, g = (ink >> 5) & 31, b = (ink >> 10) & 31;
            r = ritual_channel(r, r / 2, dim_steps, restore_steps);
            g = ritual_channel(g, g * 7 / 16, dim_steps, restore_steps);
            b = ritual_channel(b, b * 7 / 16, dim_steps, restore_steps);
            uint16_t rgb = (uint16_t)(r | (g << 5) | (b << 10));
            /* A darkened entry still occludes the room behind it at black. */
            palettes[row][i] = !rgb && ink ? UINT16_C(0x8000) : rgb;
        }
    }
}

static void triangle(const uint16_t tiles[512][1024], const Face *face,
                     const uint16_t *palette, const Point p[3], const Texel uv[3], uint32_t *pixels)
{
    Point a = p[0], b = p[1], c = p[2];
    int area = edge(a, b, c.x, c.y);
    if (area <= 0) return;
    int left = larger(0, smaller(a.x, smaller(b.x, c.x)));
    int right = smaller(SCREEN_W - 1, larger(a.x, larger(b.x, c.x)));
    int top = larger(TOP, smaller(a.y, smaller(b.y, c.y)));
    int bottom = smaller(BOTTOM - 1, larger(a.y, larger(b.y, c.y)));
    int include_a = closed_edge(b, c), include_b = closed_edge(c, a), include_c = closed_edge(a, b);
    unsigned bits = face->texture_bits, word_shift = bits == 8 ? 1 : 2;
    unsigned word_mask = (1u << word_shift) - 1, index_mask = (1u << bits) - 1;
    int modulates = face->rgb[0] != 128 || face->rgb[1] != 128 || face->rgb[2] != 128;
    uint32_t flat = 0;
    if (!bits) {
        uint16_t ink = (face->rgb[0] >> 3) | ((face->rgb[1] >> 3) << 5) |
                       ((face->rgb[2] >> 3) << 10);
        flat = color(ink);
    }
    for (int y = top; y <= bottom; ++y) {
        for (int x = left; x <= right; ++x) {
            int wa = edge(b, c, x, y), wb = edge(c, a, x, y), wc = area - wa - wb;
            if (wa < !include_a || wb < !include_b || wc < !include_c) continue;
            if (!bits) {
                pixels[y * SCREEN_W + x] = flat;
                continue;
            }
            unsigned u = (unsigned)(((int64_t)wa * uv[0].u + (int64_t)wb * uv[1].u +
                                     (int64_t)wc * uv[2].u + area / 2) / area);
            unsigned v = (unsigned)(((int64_t)wa * uv[0].v + (int64_t)wb * uv[1].v +
                                     (int64_t)wc * uv[2].v + area / 2) / area);
            uint16_t packed = tiles[face->page_y + v][(face->page_x + (u >> word_shift)) & 1023u];
            unsigned index = (packed >> (bits * (u & word_mask))) & index_mask;
            uint16_t ink = palette[index];
            if (ink) {
                int blend = face->semi_transparent && (ink & 0x8000u);
                if (modulates) {
                    unsigned modulated = 0;
                    for (unsigned channel = 0; channel < 3; ++channel) {
                        unsigned sample = ((ink >> (5*channel)) & 31u) * face->rgb[channel] / 128;
                        modulated |= (sample < 32 ? sample : 31) << (5*channel);
                    }
                    ink = (uint16_t)modulated;
                }
                if (blend)
                    ink = blend_texel(ink, pixels[y * SCREEN_W + x], face->blend_mode);
                pixels[y * SCREEN_W + x] = color(ink);
            }
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

static void draw_mesh(Palace *palace, const uint16_t tiles[512][1024], uint32_t *pixels,
                      int near_clip, const uint16_t (*ritual_palettes)[256],
                      void (*draw_to)(void *, int), void *user)
{
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
        if (near_z <= near_clip || edge(p[0], p[1], p[2].x, p[2].y) <= 0) continue;
        /* Furthest-vertex depth determines the four-unit ordering bucket.
         * Certain surfaces have an additional 512-unit draw priority. */
        palace->visible[drawn++] = (Draw){(far_z + 512 * face->depth_bias) / 4, i};
    }
    qsort(palace->visible, drawn, sizeof(*palace->visible), far_first);
    for (size_t i = 0; i < drawn; ++i) {
        if (draw_to) draw_to(user, palace->visible[i].depth);
        if (palace->actors)
            bfm_stair_actors_draw_to(palace->actors, palace->visible[i].depth, tiles, pixels);
        const Face *face = &palace->faces[palace->visible[i].index];
        const uint16_t *palette = tiles[face->palette_y] + face->palette_x;
        if (ritual_palettes && face->palette_x == 0) {
            for (unsigned row = 0; row < RITUAL_PALETTES; ++row) {
                if (face->palette_y == ritual_rows[row]) {
                    palette = ritual_palettes[row];
                    break;
                }
            }
        }
        Point p[4];
        for (unsigned j = 0; j < face->count; ++j)
            p[j] = palace->projected[face->vertices[j]];
        triangle(tiles, face, palette, p, face->uv, pixels);
        if (face->count == 4) {
            Point back[3] = {p[2], p[1], p[3]};
            Texel uv[3] = {face->uv[2], face->uv[1], face->uv[3]};
            triangle(tiles, face, palette, back, uv, pixels);
        }
    }
    if (draw_to) draw_to(user, INT_MIN);
    if (palace->actors)
        bfm_stair_actors_draw_to(palace->actors, INT_MIN, tiles, pixels);
}

void bfm_palace_draw(Palace *palace, const uint16_t tiles[512][1024],
                     unsigned field, uint32_t *pixels)
{
    if (field < 696) return;
    project_mesh(palace, field);
    uint16_t palettes[RITUAL_PALETTES][256];
    unsigned shade = palace->member && field >= 4276 ? 1 + (field - 4276) / 4 : 0;
    if (shade) {
        /* The second flash restores each channel one level per room tick. */
        unsigned restore = field >= 4634 ? 1 + (field - 4634) / 2 : 0;
        ritual_colors(tiles, (int)shade, (int)restore, palettes);
    }
    draw_mesh(palace, tiles, pixels, palace->member ? 0 : 200, shade ? palettes : NULL, NULL, NULL);
    if (palace->dialogue) bfm_palace_dialogue_draw(palace->dialogue, field, (uint32_t (*)[320])pixels);
    if (field >= 832 && field < 952)
        place_name(tiles, pixels);
}

/* Room event commands share this geometry and place the camera relative to the
 * event actor. The six camera coordinates move independently during a pan. */
struct BfmBedroomField {
    Palace geometry;
    int current[6], target[6], active;
    unsigned pan_divisor, focal;
    const uint8_t *panorama_map, *panorama_atlas;
    BfmBedroomView view;
};

static const uint8_t *field_data(const uint8_t *scene, size_t bytes,
                                 uint32_t address, size_t length)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (!scene || address < base || address - base > bytes ||
        length > bytes - (address - base)) return NULL;
    return scene + (address - base);
}

BfmBedroomField *bfm_bedroom_field_load(const uint8_t *scene, size_t bytes,
                                        uint32_t room_address, unsigned focal)
{
    const uint8_t *room = field_data(scene, bytes, room_address, 0x98);
    if (!room) return NULL;
    const uint8_t *mesh = field_data(scene, bytes, le32(room + 0x94), 20);
    if (!mesh) return NULL;
    BfmBedroomField *field = calloc(1, sizeof(*field));
    if (field && !load_mesh(&field->geometry, scene, bytes, (size_t)(mesh - scene) + 8)) {
        free(field);
        return NULL;
    }
    if (field) field->focal = focal;
    return field;
}

int bfm_bedroom_field_panorama(BfmBedroomField *field, const uint8_t *scene, size_t bytes, uint32_t room)
{
    const uint8_t *root = field_data(scene, bytes, room, 0x34);
    if (!field || !root) return 0;
    const uint8_t *map = field_data(scene,bytes,le32(root+0x30),8);
    const uint8_t *atlas = field_data(scene,bytes,le32(root+0x28),8);
    if (!map || !atlas || map[4]!=20 || map[5]!=7 || map[6]!=16 || map[7]!=16) return 0;
    unsigned count = le16(atlas+4);
    if (!count || !field_data(scene,bytes,le32(root+0x28),8+8u*count) ||
        !field_data(scene,bytes,le32(root+0x30),8+2u*map[4]*map[5])) return 0;
    for (unsigned i=0; i<(unsigned)map[4]*map[5]; ++i)
        if (le16(map+8+2*i)>=count) return 0;
    for (unsigned i=0; i<count; ++i) {
        const uint8_t *tile = atlas+8+8*i;
        if ((le16(tile+6)&0x180) || tile[0]>240 || tile[1]>240 || (le16(tile+2)>>6)>=512) return 0;
    }
    field->panorama_map=map;
    field->panorama_atlas=atlas;
    return 1;
}

static void draw_panorama(const BfmBedroomField *field, const uint16_t tiles[512][1024], uint32_t *pixels)
{
    const uint8_t *map = field->panorama_map;
    if (!map) return;
    for (unsigned y=10; y<(unsigned)map[5]*16; ++y) {
        for (unsigned col=0; col<map[4]; ++col) {
            unsigned index = le16(map+8+2*((y/16)*map[4]+col));
            const uint8_t *tile = field->panorama_atlas+8+8*index;
            unsigned page=le16(tile+6), clut=le16(tile+2);
            unsigned v=((page&16u)<<4)+tile[1]+y%16;
            const uint16_t *palette=&tiles[clut>>6][(clut&63)*16];
            for (unsigned x=0; x<16; ++x) {
                unsigned u=tile[0]+x;
                unsigned bits=tiles[v][(page&15)*64+u/4]>>(4*(u%4));
                pixels[320*y+col*16+x]=color(palette[bits&15]);
            }
        }
    }
}

void bfm_bedroom_field_free(BfmBedroomField *field)
{
    free(field);
}

/* The room camera uses the paired fixed-point rotation coefficients. A
 * reflected quarter starts with its final sample, phase 1023. Signed angles
 * select the magnitude first and change the sign of the sine. */
static int field_quarter_axis(unsigned units)
{
    unsigned phase = units & 1023u;
    if (units & 1024u) phase = 1023u - phase;
    int value = (int)lround(4096 * sin(phase * (6.28318530717958647693 / 4096)));
    return units & 2048u ? -value : value;
}

static void field_rotation(int angle, int *sine, int *cosine)
{
    unsigned magnitude = (unsigned)abs(angle);
    int value = field_quarter_axis(magnitude);
    *sine = angle < 0 ? -value : value;
    *cosine = field_quarter_axis(magnitude + 1024u);
}

static void refresh_bedroom_view(BfmBedroomField *field)
{
    const int *p = field->current;
    if (p[0] == p[3] && p[2] == p[5]) return;
    Pose pose = {{p[0], p[1], p[2]}, {p[3], p[4], p[5]}};
    field->view = make_view(pose.eye, look_at(pose, 1), field->focal);
    memcpy(field->view.target, p + 3, sizeof(field->view.target));
}

int bfm_bedroom_field_camera(BfmBedroomField *field, const uint8_t *scene,
                             size_t bytes, uint32_t camera_address,
                             const int actor_origin[3], int immediate)
{
    const uint8_t *p = field_data(scene, bytes, camera_address, 16);
    if (!field || !p || !actor_origin) return 0;
    int target[3];
    for (unsigned i = 0; i < 3; ++i) {
        if (actor_origin[i] < INT16_MIN || actor_origin[i] > INT16_MAX) return 0;
        target[i] = actor_origin[i] + signed16(p + 2*i);
    }
    return bfm_bedroom_field_orbit(field, target, signed16(p+8), signed16(p+10),
                                   signed16(p+12), immediate, 12);
}

static int position_orbit(BfmBedroomField *field, const int target[3], int yaw,
                            int distance, int pitch, int immediate, unsigned divisor)
{
    if (!field || !target || !distance || distance < -INT16_MAX || distance > INT16_MAX ||
        yaw < INT16_MIN || yaw > INT16_MAX || pitch < INT16_MIN || pitch > INT16_MAX) return 0;
    int sx, cx, sy, cy;
    field_rotation(pitch, &sx, &cx);
    field_rotation(yaw, &sy, &cy);
    int arm[3] = {floor_div((int64_t)sy * cx, 4096), sx, floor_div((int64_t)cy * cx, 4096)};
    int coordinates[6];
    for (unsigned i = 0; i < 3; ++i) {
        int aim = target[i];
        if (aim < INT16_MIN || aim > INT16_MAX) return 0;
        int eye = aim + floor_div(-(int64_t)distance * arm[i], 4096);
        if (eye < INT16_MIN || eye > INT16_MAX) return 0;
        coordinates[i] = eye;
        coordinates[i + 3] = aim;
    }
    if (coordinates[0] == coordinates[3] && coordinates[2] == coordinates[5]) return 0;
    bfm_bedroom_field_viewpoint(field, coordinates, coordinates+3, immediate, divisor);
    return 1;
}

int bfm_bedroom_field_orbit(BfmBedroomField *field, const int target[3], int yaw,
                            int distance, int pitch, int immediate, unsigned divisor)
{
    return distance > 0 && position_orbit(field, target, yaw, distance, pitch, immediate, divisor);
}

int bfm_bedroom_field_cable_view(BfmBedroomField *field, const int target[3], int yaw, int pitch)
{
    return position_orbit(field, target, yaw, -2000, -pitch, 1, 1);
}

int bfm_bedroom_field_set_focal(BfmBedroomField *field, unsigned focal)
{
    if (!field || !focal || focal > 32767) return 0;
    field->focal = focal;
    field->view.focal = (int)focal;
    return 1;
}

void bfm_bedroom_field_viewpoint(BfmBedroomField *field, const int eye[3], const int target[3],
                                 int immediate, unsigned divisor)
{
    memcpy(field->target, eye, 3*sizeof(int));
    memcpy(field->target+3, target, 3*sizeof(int));
    field->pan_divisor = divisor ? divisor : 1;
    if (!field->active || immediate) {
        memcpy(field->current, field->target, sizeof(field->current));
        refresh_bedroom_view(field);
    }
    field->active = 1;
}

void bfm_bedroom_field_step(BfmBedroomField *field)
{
    if (!field || !field->active) return;
    for (unsigned i = 0; i < 6; ++i) {
        int delta = field->target[i] - field->current[i];
        if (!delta) continue;
        int divisor = (int)field->pan_divisor;
        while (divisor > 1 && delta / divisor == 0) divisor /= 2;
        field->current[i] += delta / divisor;
    }
    refresh_bedroom_view(field);
}

const BfmBedroomView *bfm_bedroom_field_view(const BfmBedroomField *field)
{
    return field && field->active ? &field->view : NULL;
}

void bfm_bedroom_field_draw(BfmBedroomField *field, const uint16_t tiles[512][1024],
                            uint32_t *pixels, void (*draw_to)(void *, int), void *user)
{
    if (!field || !field->active || !tiles || !pixels) return;
    draw_panorama(field,tiles,pixels);
    project_vertices(&field->geometry, &field->view);
    draw_mesh(&field->geometry, tiles, pixels, 0, NULL, draw_to, user);
}

int bfm_bedroom_project(const BfmBedroomView *view, const int16_t model[3], int xy_depth[3])
{
    if (!view || !model || !xy_depth) return 0;
    Point p = project_point(view, (Point){model[0], model[1], model[2]});
    xy_depth[0] = p.x;
    xy_depth[1] = p.y;
    xy_depth[2] = p.z;
    return p.z > 0;
}
