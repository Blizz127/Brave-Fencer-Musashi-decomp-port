/* SPDX-License-Identifier: MIT */
#include "forest_terrain.h"
#include "forest_dust.h"
#include "forest_tether_trails.h"
#include "forest_muzzle_art.h"
#include "forest_shot_effects.h"
#include "scene_tiles.h"
#include "forest_raster.h"

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { GRID_CELLS = 64 * 64, TABLE_BYTES = GRID_CELLS * 4 };
typedef struct { int16_t world[3]; uint8_t u, v; } Corner;
typedef struct {
    Corner corners[4];
    uint16_t clut, page;
    uint8_t count, rgb[3], translucent;
} Face;
typedef struct {
    int16_t bounds[3][2];
    unsigned first, count;
} TerrainCell;
typedef struct { int x, y, z; } ScreenPoint;
typedef struct {
    ScreenPoint points[4];
    unsigned depth, ordinal;
    const MusashiForestPlayerFace *model_face;
    uint32_t lighting[4];
} ProjectedFace;

typedef struct {
    size_t count, cell_count;
    TerrainCell *cells;
    Face *faces;
} TerrainMesh;

struct BfmForestTerrain {
    TerrainMesh mesh;
    size_t capacity;
    int has_lighting;
    int cull_behind_heading;
    BfmForestModelLighting room_light;
    ProjectedFace *draw;
    uint16_t tiles[512][1024];
};

static uint16_t le16(const uint8_t *p)
{
    return (uint16_t)(p[0] | (unsigned)p[1] << 8);
}

static uint32_t le32(const uint8_t *p)
{
    return le16(p) | (uint32_t)le16(p + 2) << 16;
}

static int decode_face(Face *face, const uint8_t *record, const uint8_t *vertices,
                       size_t vertex_count, const uint8_t *materials, size_t material_bytes, int shared_vertices)
{
    uint32_t ref = le32(record);
    size_t off = ref & 0xffffff;
    uint16_t last = le16(record + 10);
    /* Sparse cells can mark the same textured triangle or quad with bit zero.
     * The mounted polygon command and the aligned fourth index identify its shape. */
    unsigned mode = last & (shared_vertices ? 7u : 14u);
    if ((off & 15) || off >= material_bytes || (mode != 2 && mode != 6))
        return 0;
    const uint8_t *mat = materials + off;
    face->count = mode == 2 ? 4 : 3;
    if (mat[3] != (ref >> 24) || (mat[3] & ~2) != (face->count == 4 ? 0x2c : 0x24))
        return 0;
    face->translucent = (mat[3] & 2) != 0;
    memcpy(face->rgb, mat, sizeof(face->rgb));
    face->clut = le16(mat + 6);
    face->page = le16(mat + 10);
    if (((face->page >> 7) & 3) > 2)
        return 0;
    static const uint8_t uv_offset[] = {4, 8, 12, 14};
    for (unsigned i = 0; i < face->count; ++i) {
        unsigned vertex = i < 3 ? le16(record + 4 + 2*i) :
            shared_vertices ? (last & ~7u) : (last >> 4) * 8u;
        if ((vertex & 7) || vertex / 8 >= vertex_count)
            return 0;
        for (unsigned axis = 0; axis < 3; ++axis)
            face->corners[i].world[axis] = (int16_t)le16(vertices + vertex + axis * 2);
        face->corners[i].u = mat[uv_offset[i]];
        face->corners[i].v = mat[uv_offset[i] + 1];
    }
    return 1;
}

/* Interior sectors share a material and vertex pool; each directory entry
 * owns a contiguous face run and signed world bounding box. */
static int load_grouped_faces(TerrainMesh *terrain, const uint8_t *mesh, size_t bytes)
{
    if (bytes < 20) return 0;
    const uint8_t *root = mesh + 20;
    size_t size = bytes - 20;
    unsigned groups = le32(mesh + 8);
    size_t material = le32(mesh + 12), vertex = le32(mesh + 16);
    if (!groups || groups > size / 20 || material != 20u * groups || vertex < material ||
        vertex > size || (vertex - material) % 16) return 0;
    size_t first = le32(root + 16);
    if (first < vertex || first > size || (first - vertex) % 8) return 0;
    size_t cursor = first;
    for (unsigned i = 0; i < groups; ++i) {
        const uint8_t *entry = root + 20 * i;
        size_t count = le32(entry + 12);
        if (le32(entry + 16) != cursor || count > (size - cursor) / 12) return 0;
        cursor += count * 12;
    }
    terrain->cell_count = groups;
    terrain->count = (cursor - first) / 12;
    if (!terrain->count) return 0;
    terrain->cells = calloc(groups, sizeof *terrain->cells);
    terrain->faces = calloc(terrain->count, sizeof *terrain->faces);
    if (!terrain->cells || !terrain->faces) return 0;
    unsigned used = 0;
    for (unsigned i = 0; i < groups; ++i) {
        const uint8_t *entry = root + 20 * i;
        TerrainCell *cell = &terrain->cells[i];
        for (unsigned a = 0; a < 3; ++a) {
            cell->bounds[a][0] = (int16_t)le16(entry + 4*a);
            cell->bounds[a][1] = (int16_t)le16(entry + 4*a + 2);
            if (cell->bounds[a][0] > cell->bounds[a][1]) return 0;
        }
        cell->first = used;
        cell->count = le32(entry + 12);
        for (unsigned j = 0; j < cell->count; ++j, ++used) {
            const uint8_t *face = root + first + 12 * used;
            if (!decode_face(&terrain->faces[used], face, root + vertex, (first - vertex) / 8,
                             root + material, vertex - material, 1)) return 0;
        }
    }
    return 1;
}

static int load_faces(TerrainMesh *terrain, const uint8_t *mesh, size_t bytes)
{
    if (bytes >= 8 && le32(mesh) == 0x43 && !le32(mesh + 4))
        return load_grouped_faces(terrain, mesh, bytes);
    if (bytes < 8 + TABLE_BYTES || mesh[0] != 0x42 || mesh[4])
        return 0;
    const uint8_t *table = mesh + 8;
    const uint8_t *materials = table + TABLE_BYTES;
    size_t material_bytes = (size_t)le16(mesh + 6) * 16;
    size_t end = bytes - 8;
    if (material_bytes > end - TABLE_BYTES)
        return 0;
    /* Every occupied entry owns the bytes up to the next occupied entry.
     * Validate all spans before sizing the host's draw buffers. */
    for (unsigned i = GRID_CELLS; i > 0; --i) {
        size_t off = le32(table + 4 * (i-1));
        if (!off)
            continue;
        if ((off & 3) || off < TABLE_BYTES + material_bytes || off > end || end - off < 28)
            return 0;
        const uint8_t *cell = table + off;
        size_t v = le32(cell), n = le32(cell + 4);
        size_t f = le32(cell + 20), count = le32(cell + 24);
        if (v < 28 || (v & 3) || (f & 3) || v > f || f > end - off ||
            n > (f-v)/8 || count > (end-off-f)/12)
            return 0;
        ++terrain->cell_count;
        terrain->count += count;
        end = off;
    }
    if (!terrain->count || terrain->count > UINT_MAX ||
        terrain->count > SIZE_MAX / sizeof(*terrain->faces))
        return 0;
    terrain->cells = calloc(terrain->cell_count, sizeof(*terrain->cells));
    terrain->faces = calloc(terrain->count, sizeof(*terrain->faces));
    if (!terrain->cells || !terrain->faces)
        return 0;
    size_t used = 0, occupied = 0;
    for (unsigned i = 0; i < GRID_CELLS; ++i) {
        size_t off = le32(table + 4*i);
        if (!off)
            continue;
        const uint8_t *cell = table + off;
        const uint8_t *vertices = cell + le32(cell), *faces = cell + le32(cell + 20);
        TerrainCell *group = &terrain->cells[occupied++];
        group->first = (unsigned)used;
        group->count = le32(cell + 24);
        for (unsigned axis = 0; axis < 3; ++axis) {
            group->bounds[axis][0] = (int16_t)le16(cell + 8 + 4*axis);
            group->bounds[axis][1] = (int16_t)le16(cell + 10 + 4*axis);
            if (group->bounds[axis][0] > group->bounds[axis][1])
                return 0;
        }
        for (unsigned j = 0; j < group->count; ++j) {
            if (!decode_face(&terrain->faces[used++], faces + 12*j, vertices,
                             le32(cell + 4), materials, material_bytes, 0))
                return 0;
        }
    }
    return 1;
}

/* The field quantizes the squared length to the leading eight bits in
 * powers of four before taking the integer square root. */
static unsigned field_length(uint32_t squared)
{
    unsigned shift = 0;
    while ((squared >> shift) > 255)
        shift += 2;
    return (unsigned)sqrt((double)((squared >> shift) << shift));
}

void bfm_forest_terrain_set_forward_culling(BfmForestTerrain *terrain, int enabled)
{
    if (terrain)
        terrain->cull_behind_heading = enabled != 0;
}

int bfm_forest_terrain_set_lighting(BfmForestTerrain *terrain, const uint8_t *room, size_t bytes)
{
    if (!terrain || !room || bytes < 36)
        return 0;
    terrain->has_lighting = 0;
    for (unsigned i = 0; i < 3; ++i) {
        int axis[3];
        uint32_t squared = 0;
        const uint8_t *light = room + 6 + i*10;
        for (unsigned k = 0; k < 3; ++k) {
            axis[k] = (int16_t)le16(light + k*2);
            squared += (uint32_t)(axis[k]*axis[k]);
        }
        unsigned length = field_length(squared);
        if (!length)
            return 0;
        for (unsigned k = 0; k < 3; ++k) {
            terrain->room_light.direction[i][k] = -axis[k] * 4096 / (int)length;
            terrain->room_light.color[k][i] = light[6+k] * 4096 / 255;
        }
        int ambient = (int16_t)le16(room + 2*i);
        terrain->room_light.ambient[i] = ambient - ((ambient % 16 + 16) % 16);
    }
    terrain->has_lighting = 1;
    return 1;
}

static int reserve_faces(BfmForestTerrain *terrain, size_t required)
{
    if (required <= terrain->capacity) return 1;
    if (required > SIZE_MAX / sizeof(*terrain->draw)) return 0;
    ProjectedFace *faces = realloc(terrain->draw, required * sizeof(*faces));
    if (!faces) return 0;
    terrain->draw = faces;
    terrain->capacity = required;
    return 1;
}

static void free_mesh(TerrainMesh *mesh)
{
    free(mesh->cells);
    free(mesh->faces);
}

int bfm_forest_terrain_replace_mesh(BfmForestTerrain *terrain, const uint8_t *mesh, size_t bytes)
{
    if (!terrain || !mesh) return 0;
    TerrainMesh next = {0};
    if (!load_faces(&next, mesh, bytes) || !reserve_faces(terrain, next.count)) {
        free_mesh(&next);
        return 0;
    }
    free_mesh(&terrain->mesh);
    terrain->mesh = next;
    return 1;
}

BfmForestTerrain *bfm_forest_terrain_load_archive(const MusashiDiscMedia *disc, const char *archive, unsigned member,
                                          const uint8_t *mesh, size_t bytes)
{
    if (!disc || !archive || !mesh)
        return NULL;
    BfmForestTerrain *terrain = calloc(1, sizeof(*terrain));
    if (terrain && (!bfm_forest_terrain_replace_mesh(terrain, mesh, bytes) ||
                    !bfm_scene_read_tiles(disc, "MAIN.CD", 10, terrain->tiles) ||
                    !bfm_scene_read_tiles(disc, archive, member, terrain->tiles))) {
        bfm_forest_terrain_free(terrain);
        return NULL;
    }
    return terrain;
}

BfmForestTerrain *bfm_forest_terrain_load_member(const MusashiDiscMedia *disc, unsigned member,
                                          const uint8_t *mesh, size_t bytes)
{
    return bfm_forest_terrain_load_archive(disc, "SC01.CD", member, mesh, bytes);
}

BfmForestTerrain *bfm_forest_terrain_load(const MusashiDiscMedia *disc,
                                          const uint8_t *mesh, size_t bytes)
{
    return bfm_forest_terrain_load_member(disc, 77, mesh, bytes);
}

const uint16_t (*bfm_forest_terrain_tiles(const BfmForestTerrain *terrain))[1024]
{
    return terrain ? terrain->tiles : NULL;
}

void bfm_forest_terrain_free(BfmForestTerrain *terrain)
{
    if (terrain) {
        free_mesh(&terrain->mesh);
        free(terrain->draw);
        free(terrain);
    }
}

static int smaller(int a, int b) { return a < b ? a : b; }
static int larger(int a, int b) { return a > b ? a : b; }

static int64_t floor_denom(int64_t n, int d)
{
    return n / d - (n % d < 0);
}

int bfm_forest_camera_look_at(const int32_t eye[3], const int32_t target[3],
                             uint16_t projection, BfmForestCamera *out)
{
    if (!eye || !target || !projection || !out)
        return 0;
    int delta[3];
    uint32_t total = 0;
    for (unsigned i = 0; i < 3; ++i) {
        int64_t difference = (int64_t)target[i] - eye[i];
        /* This camera's short-axis path handles a subject in the field's
         * nearby follow volume. Larger cinematics use other angle logic. */
        if (difference < -724 || difference > 724)
            return 0;
        delta[i] = (int)difference;
        total += (unsigned)(difference * difference);
    }
    unsigned ground_square = (unsigned)(delta[0]*delta[0] + delta[2]*delta[2]);
    if (!ground_square || ground_square >= 524288)
        return 0;
    int ground = (int)field_length(ground_square), length = (int)field_length(total);
    int cy = delta[2]*4096/ground, sy = -delta[0]*4096/ground;
    int cp = ground*4096/length, sp = delta[1]*4096/length;
    BfmForestCamera view = {
        .rotation = {{(int16_t)cy, 0, (int16_t)sy},
                     {(int16_t)floor_denom((int64_t)sp*sy,4096), (int16_t)cp,
                      (int16_t)floor_denom(-(int64_t)sp*cy,4096)},
                     {(int16_t)floor_denom(-(int64_t)cp*sy,4096), (int16_t)sp,
                      (int16_t)floor_denom((int64_t)cp*cy,4096)}},
        .projection = projection
    };
    for (unsigned i = 0; i < 3; ++i) {
        int64_t position = 0;
        for (unsigned j = 0; j < 3; ++j)
            position -= (int64_t)eye[j] * view.rotation[i][j];
        position = floor_denom(position,4096);
        if (position < INT32_MIN || position > INT32_MAX)
            return 0;
        view.translation[i] = (int32_t)position;
    }
    *out = view;
    return 1;
}

/* The perspective divider normalizes the 16-bit denominator. Its seed uses
 * nine high bits, followed by one fixed-point reciprocal refinement. This
 * retains the screen-space rounding at shallow and oblique terrain edges. */
static int perspective_scale(unsigned focal, unsigned depth)
{
    unsigned scaled_focal = focal;
    while (depth < 32768) {
        depth *= 2;
        scaled_focal *= 2;
    }
    unsigned slot = (depth - 32704) / 128;
    unsigned seed = (262144 / (slot + 256) + 1) / 2;
    if (seed < 257)
        seed = 257;
    unsigned error = (33554560 - depth * seed) / 256;
    unsigned reciprocal = (128 + error * seed) / 256;
    uint64_t result = ((uint64_t)scaled_focal * reciprocal + 32768) / 65536;
    return result > 131071 ? 131071 : (int)result;
}

static int64_t view_axis(const BfmForestCamera *camera, const int16_t xyz[3], unsigned axis)
{
    int64_t dot = 0;
    for (unsigned k = 0; k < 3; ++k)
        dot += (int64_t)camera->rotation[axis][k] * xyz[k];
    return floor_denom(dot, 4096) + camera->translation[axis];
}

/* Test mounted cell bounds before perspective division or screen-coordinate
 * folding. A cell crossing a view plane can still contribute folded faces. */
static int cell_in_view(const TerrainCell *cell, const BfmForestCamera *camera)
{
    enum { LEFT = 1, RIGHT = 2, TOP = 4, BOTTOM = 8, BEHIND = 16 };
    unsigned outside = LEFT | RIGHT | TOP | BOTTOM | BEHIND;
    for (unsigned corner = 0; corner < 8 && outside; ++corner) {
        int16_t point[3];
        for (unsigned axis = 0; axis < 3; ++axis)
            point[axis] = cell->bounds[axis][(corner >> axis) & 1];
        int64_t x = view_axis(camera, point, 0) * camera->projection;
        int64_t y = view_axis(camera, point, 1) * camera->projection;
        int64_t z = view_axis(camera, point, 2);
        unsigned planes = 0;
        if (x < -160*z) planes |= LEFT;
        if (x > 160*z) planes |= RIGHT;
        if (y < -120*z) planes |= TOP;
        if (y > 120*z) planes |= BOTTOM;
        if (z < 0) planes |= BEHIND;
        outside &= planes;
    }
    return !outside;
}

static int project(const BfmForestCamera *camera, const int16_t xyz[3], ScreenPoint *out)
{
    int view[3];
    for (unsigned axis = 0; axis < 3; ++axis) {
        int64_t result = view_axis(camera, xyz, axis);
        if (result < INT16_MIN || result > INT16_MAX)
            return 0;
        view[axis] = (int)result;
    }
    int depth = view[2];
    if (depth < 0 || depth >= 16384)
        return 0;
    /* Terrain accepts the divider's saturated two-times magnification for
     * close vertices. The actor sorter applies a separate near threshold. */
    int multiplier = camera->projection >= 2*depth ? 131071 :
                     perspective_scale(camera->projection, (unsigned)depth);
    int x = (int)floor_denom((int64_t)view[0] * multiplier, 65536);
    int y = (int)floor_denom((int64_t)view[1] * multiplier, 65536);
    if (x < -1024 || x > 1023 || y < -1024 || y > 1023)
        return 0;
    *out = (ScreenPoint){160 + x, 120 + y, depth};
    return 1;
}

int bfm_forest_project_effect_quad(const BfmForestCamera *camera,
                                   const int16_t corners[4][3], BfmForestDustCard *card)
{
    if (!camera || !camera->projection || !corners || !card) return 0;
    ScreenPoint point;
    for (unsigned i = 0; i < 4; ++i) {
        if (!project(camera, corners[i], &point) || camera->projection >= 2 * point.z) return 0;
        card->vertices[i].x = point.x;
        card->vertices[i].y = point.y;
    }
    card->bucket = (unsigned)point.z / 4;
    return 1;
}

static int cross(ScreenPoint a, ScreenPoint b, int x, int y)
{
    return (b.x-a.x) * (y-a.y) - (b.y-a.y) * (x-a.x);
}

static void draw_face(const BfmForestRaster *surface, const Face *face,
                      const ScreenPoint *points, unsigned buffer)
{
    const BfmForestRasterMaterial material = {
        .page = face->page, .clut = face->clut, .count = face->count,
        .textured = 1, .translucent = face->translucent
    };
    uint32_t light = face->rgb[0] | (uint32_t)face->rgb[1] << 8 |
                     (uint32_t)face->rgb[2] << 16;
    BfmForestRasterVertex vertices[4];
    for (unsigned i = 0; i < face->count; ++i) {
        vertices[i] = (BfmForestRasterVertex){
            points[i].x - 160, points[i].y - 120, face->corners[i].u, face->corners[i].v, light
        };
    }
    bfm_forest_raster_buffer_face(surface, &material, vertices, buffer);
}

static int compose(const BfmForestTransform *parent, const BfmForestTransform *child,
                   BfmForestTransform *result)
{
    for (unsigned i = 0; i < 3; ++i) {
        int64_t offset = 0;
        for (unsigned j = 0; j < 3; ++j) {
            int64_t dot = 0;
            offset += (int64_t)parent->rotation[i][j] * child->translation[j];
            for (unsigned k = 0; k < 3; ++k)
                dot += (int64_t)parent->rotation[i][k] * child->rotation[k][j];
            int64_t v = floor_denom(dot,4096);
            if (v < INT16_MIN || v > INT16_MAX) return 0;
            result->rotation[i][j] = (int16_t)v;
        }
        offset = parent->translation[i] + floor_denom(offset,4096);
        if (offset < INT32_MIN || offset > INT32_MAX) return 0;
        result->translation[i] = (int32_t)offset;
    }
    return 1;
}

int bfm_forest_effect_camera(const BfmForestCamera *camera, const BfmForestTransform *model,
                             BfmForestCamera *local)
{
    if (!camera || !camera->projection || !model || !local) return 0;
    BfmForestTransform view, eye;
    memcpy(view.rotation, camera->rotation, sizeof view.rotation);
    memcpy(view.translation, camera->translation, sizeof view.translation);
    if (!compose(&view, model, &eye)) return 0;
    memcpy(local->rotation, eye.rotation, sizeof local->rotation);
    memcpy(local->translation, eye.translation, sizeof local->translation);
    local->projection = camera->projection;
    return 1;
}

static uint32_t light_vertex(const BfmForestModelLighting *lighting, const int32_t light[3][3],
                             const MusashiForestPlayerVector *normal, const uint8_t rgb[3])
{
    int scale[3];
    const int n[3] = {normal->x, normal->y, normal->z};
    for (unsigned i = 0; i < 3; ++i) {
        int64_t dot = 0;
        for (unsigned k = 0; k < 3; ++k)
            dot += (int64_t)light[i][k] * n[k];
        scale[i] = smaller(32767, larger(0, (int)floor_denom(dot,4096)));
    }
    uint32_t color = 0;
    for (unsigned i = 0; i < 3; ++i) {
        int64_t sum = 0;
        for (unsigned k = 0; k < 3; ++k)
            sum += (int64_t)lighting->color[i][k] * scale[k];
        int level = smaller(32767, larger(0, lighting->ambient[i] + (int)floor_denom(sum,4096)));
        color |= (uint32_t)smaller(255, rgb[i] * level / 4096) << (8*i);
    }
    return color;
}

/* At a shared depth the field submits dust and the notice before body and terrain faces.
 * The blade glow follows those faces. The pointers identify these two roles. */
static const MusashiForestPlayerFace effect_face = {.corners = 4};
static const MusashiForestPlayerFace blade_face = {.mode = 0x3a, .corners = 4, .tpage = 0x12a};

static int collect_blade(BfmForestTerrain *terrain, const BfmForestCamera *camera,
                         const BfmForestBlade *blade, const BfmForestBladeFrame *frame,
                         size_t *count)
{
    BfmForestCamera local;
    if (!bfm_forest_effect_camera(camera, &frame->world, &local) || *count > SIZE_MAX - 8 ||
        !reserve_faces(terrain, *count + 8)) return 0;
    for (unsigned id = 0; id < 8; ++id) {
        ProjectedFace *face = &terrain->draw[*count];
        int16_t xyz[4][3];
        if (!bfm_forest_blade_quad(blade, id, frame->tick, xyz, face->lighting)) return 0;
        unsigned i = 0;
        while (i < 4 && project(&local, xyz[i], &face->points[i])) ++i;
        if (i < 4) continue;
        face->depth = (unsigned)face->points[3].z / 4;
        face->ordinal = id;
        face->model_face = &blade_face;
        ++*count;
    }
    return 1;
}

typedef enum { FOREST_PLAYER_MODEL, FOREST_ROOM_MODEL } ForestModelRole;

static const MusashiForestPlayerFace white_triangle = {.corners = 3};
static const MusashiForestPlayerFace white_quad = {.corners = 4};

static uint32_t increase_modulation(uint32_t rgb, unsigned extra)
{
    uint32_t lit = 0;
    for (unsigned shift = 0; shift < 24; shift += 8) {
        unsigned level = ((rgb >> shift) & 255) + extra;
        lit |= (level < 255 ? level : 255) << shift;
    }
    return lit;
}

static int collect_model(BfmForestTerrain *terrain, const BfmForestModelLighting *lighting,
                         const BfmForestCamera *camera,
                         const MusashiForestPlayerMesh *model, const BfmForestPose *pose,
                         const BfmForestTransform *root, uint32_t mask,
                         ForestModelRole role, size_t *count)
{
    uint32_t white_parts = musashi_forest_player_mesh_white_parts(model);
    BfmForestTransform view, model_view;
    memcpy(view.rotation, camera->rotation, sizeof(view.rotation));
    memcpy(view.translation, camera->translation, sizeof(view.translation));
    if (!compose(&view,root,&model_view))
        return 0;
    for (unsigned i = 0; i < MUSASHI_FOREST_PLAYER_PARTS; ++i) {
        if (!(mask & (UINT32_C(1) << i))) continue;
        const MusashiForestPlayerPart *part = musashi_forest_player_mesh_part(model,i);
        if (!part) return 0;
        unsigned extra = musashi_forest_player_mesh_extra_light(model, i);
        size_t required = *count + part->face_count;
        if (required < *count || !reserve_faces(terrain, required)) return 0;
        BfmForestTransform world, eye;
        if (!compose(root,&pose->local[i],&world) || !compose(&model_view,&pose->local[i],&eye))
            return 0;
        BfmForestCamera part_camera;
        memcpy(part_camera.rotation, eye.rotation, sizeof(eye.rotation));
        memcpy(part_camera.translation, eye.translation, sizeof(eye.translation));
        part_camera.projection = camera->projection;
        int32_t light[3][3];
        for (unsigned a = 0; a < 3; ++a)
            for (unsigned b = 0; b < 3; ++b) {
                int64_t dot = 0;
                for (unsigned k = 0; k < 3; ++k)
                    dot += (int64_t)lighting->direction[a][k] * world.rotation[k][b];
                light[a][b] = (int32_t)floor_denom(dot,4096);
            }
        for (size_t f = 0; f < part->face_count; ++f) {
            const MusashiForestPlayerFace *face = &part->faces[f];
            ProjectedFace *out = &terrain->draw[*count];
            unsigned nearest = UINT_MAX, sum = 0, j = 0;
            unsigned near_count = (i >= 2 && (face->mode == 0x38 || face->mode == 0x3c)) ? 3 : face->corners;
            for (; j < face->corners; ++j) {
                const MusashiForestPlayerVector *v = &part->vertices[face->vertex[j]];
                const int16_t xyz[3] = {v->x,v->y,v->z};
                if (!project(&part_camera,xyz,&out->points[j]))
                    break;
                unsigned depth = (unsigned)out->points[j].z;
                if (j < near_count && depth <= 175) break;
                if (depth < nearest) nearest = depth;
                sum += depth;
            }
            if (j != face->corners ||
                cross(out->points[0],out->points[1],out->points[2].x,out->points[2].y) <= 0)
                continue;
            for (unsigned k = 0; k < ((face->mode & 16) ? face->corners : 1); ++k) {
                const uint8_t *rgb = face->corner_rgb[k];
                out->lighting[k] = (face->mode & 1) ?
                    (unsigned)rgb[0] | (unsigned)rgb[1] << 8 | (unsigned)rgb[2] << 16 :
                    light_vertex(lighting, light, &part->normals[face->normal[k]], rgb);
                if (extra && (face->mode & 4))
                    out->lighting[k] = increase_modulation(out->lighting[k], extra);
            }
            if (role == FOREST_PLAYER_MODEL && i < 2)
                out->depth = nearest/4 - 1;
            else
                out->depth = (sum * (face->corners == 3 ? 341 : 256))/4096;
            out->ordinal = (unsigned)(terrain->mesh.count + *count);
            if (white_parts & (UINT32_C(1) << i)) {
                out->lighting[0] = UINT32_C(0xffffff);
                out->model_face = face->corners == 3 ? &white_triangle : &white_quad;
            } else {
                out->model_face = face;
            }
            ++*count;
        }
    }
    return 1;
}

static void draw_model_face(const BfmForestRaster *surface, const ProjectedFace *out)
{
    const MusashiForestPlayerFace *face = out->model_face;
    const BfmForestRasterMaterial material = {
        .page = face->tpage, .clut = face->clut, .count = face->corners,
        .textured = (face->mode & 4) != 0, .translucent = (face->mode & 2) != 0
    };
    BfmForestRasterVertex vertices[4];
    for (unsigned i = 0; i < face->corners; ++i) {
        vertices[i] = (BfmForestRasterVertex){
            out->points[i].x, out->points[i].y, face->u[i], face->v[i],
            out->lighting[(face->mode & 16) ? i : 0]
        };
    }
    bfm_forest_raster_face(surface, &material, vertices);
}

static int by_depth(const void *left, const void *right)
{
    const ProjectedFace *a = left, *b = right;
    if (a->depth != b->depth)
        return a->depth > b->depth ? -1 : 1;
    if ((a->model_face == &effect_face) != (b->model_face == &effect_face))
        return a->model_face == &effect_face ? -1 : 1;
    if ((a->model_face == &blade_face) != (b->model_face == &blade_face))
        return a->model_face == &blade_face ? 1 : -1;
    return a->ordinal > b->ordinal ? -1 : a->ordinal < b->ordinal;
}

int bfm_forest_terrain_draw_player_only(BfmForestTerrain *terrain, const BfmForestCamera *camera,
                                      const MusashiForestPlayerMesh *model, const BfmForestPose *pose,
                                      const BfmForestTransform *root, uint32_t mask,
                                      const BfmForestModelLighting *lighting, uint32_t pixels[240][320])
{
    if (!terrain || !terrain->has_lighting || !camera || !camera->projection ||
        !model || !pose || !root || !pixels) return 0;
    size_t count = 0;
    if (!collect_model(terrain, lighting ? lighting : &terrain->room_light, camera, model,
                       pose, root, mask, FOREST_PLAYER_MODEL, &count))
        return 0;
    const BfmForestRaster surface = {
        .tiles = (const uint16_t (*)[1024])terrain->tiles, .pixels = pixels,
        .top = 10, .bottom = 229
    };
    qsort(terrain->draw, count, sizeof(*terrain->draw), by_depth);
    for (size_t i = 0; i < count; ++i)
        draw_model_face(&surface, &terrain->draw[i]);
    return 1;
}

int bfm_forest_terrain_draw_actor(BfmForestTerrain *terrain, const BfmForestCamera *camera,
                                  const MusashiForestPlayerMesh *model, const BfmForestPose *pose,
                                  const BfmForestTransform *root, uint32_t mask,
                                  uint32_t pixels[240][320])
{
    return bfm_forest_terrain_draw_actor_buffer(terrain, camera, model, pose, root,
                                                mask, 0, pixels);
}

/* The GPU folds screen coordinates to a signed 11-bit range after adding
 * the draw-buffer origin. The Z and facing tests use the projected corners
 * before the fold. */
static int signed_screen(int coordinate)
{
    return (int)(((unsigned)coordinate + 1024u) & 2047u) - 1024;
}

int bfm_forest_terrain_draw_actor_buffer(BfmForestTerrain *terrain, const BfmForestCamera *camera,
                                  const MusashiForestPlayerMesh *model, const BfmForestPose *pose,
                                  const BfmForestTransform *root, uint32_t mask, unsigned buffer,
                                  uint32_t pixels[240][320])
{
    return bfm_forest_terrain_draw_entrance(terrain, camera, model, pose, root, mask, NULL,
                                           buffer, NULL, pixels);
}

/* For a face that wraps vertically into the viewport, test its world
 * position against the camera's forward direction on the ground plane.
 * A steep view can still see a folded face that crosses this plane. */
static int behind_camera_heading(const Face *face, const BfmForestCamera *camera)
{
    int fx = camera->rotation[2][0], fz = camera->rotation[2][2];
    int64_t eye_depth = 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        int64_t horizontal = (int64_t)camera->rotation[axis][0] * fx +
                             (int64_t)camera->rotation[axis][2] * fz;
        eye_depth -= horizontal * camera->translation[axis];
    }
    for (unsigned i = 0; i < face->count; ++i) {
        const int16_t *xyz = face->corners[i].world;
        int64_t depth = (int64_t)xyz[0] * fx + (int64_t)xyz[2] * fz;
        if (depth * 4096 >= eye_depth)
            return 0;
    }
    return 1;
}

static size_t collect_cell(BfmForestTerrain *terrain, const TerrainCell *cell,
                           const BfmForestCamera *camera, unsigned buffer, size_t count)
{
    for (unsigned i = cell->first; i < cell->first + cell->count; ++i) {
        const Face *face = &terrain->mesh.faces[i];
        ProjectedFace *out = &terrain->draw[count];
        unsigned depth = 0, j = 0;
        int raw_min_x = 320, raw_max_x = -1;
        int raw_min_y = 240, raw_max_y = -1;
        for (; j < face->count; ++j) {
            ScreenPoint *point = &out->points[j];
            if (!project(camera, face->corners[j].world, point))
                break;
            if ((unsigned)point->z > depth)
                depth = (unsigned)point->z;
            raw_min_x = smaller(raw_min_x, point->x);
            raw_max_x = larger(raw_max_x, point->x);
            raw_min_y = smaller(raw_min_y, point->y);
            raw_max_y = larger(raw_max_y, point->y);
        }
        /* The room contains horizontally remote cells outside the submitted
         * doorway geometry. Keep an offscreen lateral triangle from folding
         * back across the player once its draw offset has been added. */
        if (j != face->count || raw_min_x >= 320 || raw_max_x < 0 ||
            cross(out->points[0], out->points[1], out->points[2].x, out->points[2].y) <= 0)
            continue;
        int x0 = 320, y0 = 240, x1 = -1, y1 = -1;
        int origin_y = (int)buffer * 240;
        for (unsigned k = 0; k < face->count; ++k) {
            const ScreenPoint *point = &out->points[k];
            int x = signed_screen(point->x);
            int y = signed_screen(point->y + origin_y) - origin_y;
            x0 = smaller(x0, x);
            x1 = larger(x1, x);
            y0 = smaller(y0, y);
            y1 = larger(y1, y);
        }
        if (x0 >= 320 || x1 < 0 || y0 >= 240 || y1 < 0)
            continue;
        if (terrain->cull_behind_heading && (raw_min_y >= 240 || raw_max_y < 0) &&
            behind_camera_heading(face, camera))
            continue;
        out->depth = depth / 4;
        out->ordinal = i;
        out->model_face = NULL;
        ++count;
    }
    return count;
}

int bfm_forest_terrain_draw_entrance(BfmForestTerrain *terrain, const BfmForestCamera *camera,
                                  const MusashiForestPlayerMesh *model, const BfmForestPose *pose,
                                  const BfmForestTransform *root, uint32_t mask, const BfmForestBlade *blade, unsigned buffer,
                                  const BfmForestBladeFrame *frame,
                                  uint32_t pixels[240][320])
{
    return bfm_forest_terrain_draw_entrance_with_actor(terrain, camera, model, pose, root,
                                                      mask, blade, buffer, frame, NULL, pixels);
}

int bfm_forest_terrain_draw_entrance_with_actor(BfmForestTerrain *terrain,
                                  const BfmForestCamera *camera,
                                  const MusashiForestPlayerMesh *model, const BfmForestPose *pose,
                                  const BfmForestTransform *root, uint32_t mask,
                                  const BfmForestBlade *blade, unsigned buffer,
                                  const BfmForestBladeFrame *frame, const BfmForestActor *actor,
                                  uint32_t pixels[240][320])
{
    return bfm_forest_terrain_draw_entrance_with_actors(terrain, camera, model, pose, root,
                                                       mask, blade, buffer, frame, actor,
                                                       actor ? 1u : 0u, pixels);
}

int bfm_forest_terrain_draw_entrance_with_actors(BfmForestTerrain *terrain,
                                  const BfmForestCamera *camera,
                                  const MusashiForestPlayerMesh *model, const BfmForestPose *pose,
                                  const BfmForestTransform *root, uint32_t mask,
                                  const BfmForestBlade *blade, unsigned buffer,
                                  const BfmForestBladeFrame *frame, const BfmForestActor *actors,
                                  size_t actor_count, uint32_t pixels[240][320])
{
    return bfm_forest_terrain_draw_entrance_with_effects(terrain, camera, model, pose, root,
                    mask, blade, buffer, frame, actors, actor_count, NULL, 0, pixels);
}

int bfm_forest_terrain_draw_entrance_with_effects(BfmForestTerrain *terrain,
                                  const BfmForestCamera *camera,
                                  const MusashiForestPlayerMesh *model, const BfmForestPose *pose,
                                  const BfmForestTransform *root, uint32_t mask,
                                  const BfmForestBlade *blade, unsigned buffer,
                                  const BfmForestBladeFrame *frame, const BfmForestActor *actors,
                                  size_t actor_count, const BfmForestDustCard *cards,
                                  size_t card_count, uint32_t pixels[240][320])
{
    if (!terrain || !camera || !camera->projection || !pixels || buffer > 1 ||
        actor_count > BFM_FOREST_ACTOR_LIMIT || (actor_count && !actors) ||
        card_count > BFM_FOREST_EFFECT_CARDS + BFM_TETHER_CARDS +
                     BFM_FOREST_MUZZLE_CARDS + BFM_GUNSHOT_FAN_CARDS + BFM_FOREST_SHOT_EFFECT_CARDS ||
        (card_count && !cards) ||
        ((model || actor_count) && !terrain->has_lighting))
        return 0;
    /* The actor uses the 220-line gameplay viewport. Terrain diagnostics
     * render all rows of the frame. */
    const BfmForestRaster surface = {
        .tiles = (const uint16_t (*)[1024])terrain->tiles, .pixels = pixels,
        .top = model || actor_count ? 10 : 0, .bottom = model || actor_count ? 229 : 239
    };
    size_t count = 0;
    for (size_t i = 0; i < terrain->mesh.cell_count; ++i) {
        const TerrainCell *cell = &terrain->mesh.cells[i];
        if (cell_in_view(cell, camera))
            count = collect_cell(terrain, cell, camera, buffer, count);
    }
    if (model && (!pose || !root ||
                  !collect_model(terrain, &terrain->room_light, camera, model, pose, root, mask, FOREST_PLAYER_MODEL, &count)))
        return 0;
    for (size_t i = 0; i < actor_count; ++i) {
        const BfmForestActor *actor = &actors[i];
        if (!actor->mesh || !actor->pose || !actor->root ||
            (actor->visible_parts >> MUSASHI_FOREST_PLAYER_PARTS) ||
            !collect_model(terrain, &terrain->room_light, actor->submitted_camera ? actor->submitted_camera : camera,
                           actor->mesh, actor->pose, actor->root,
                           actor->visible_parts, FOREST_ROOM_MODEL, &count)) return 0;
    }
    if (frame && (!blade || !collect_blade(terrain, camera, blade, frame, &count))) return 0;
    if (card_count > SIZE_MAX - count || !reserve_faces(terrain, count + card_count)) return 0;
    for (size_t i = 0; i < card_count; ++i) {
        ProjectedFace *out = &terrain->draw[count++];
        out->depth = cards[i].bucket;
        out->ordinal = (unsigned)i;
        out->model_face = &effect_face;
    }
    qsort(terrain->draw, count, sizeof(*terrain->draw), by_depth);
    for (size_t i = 0; i < count; ++i) {
        ProjectedFace *out = &terrain->draw[i];
        if (out->model_face == &effect_face) {
            const BfmForestDustCard *card = &cards[out->ordinal];
            bfm_forest_raster_face(&surface, &card->material, card->vertices);
        } else if (out->model_face)
            draw_model_face(&surface, out);
        else
            draw_face(&surface, &terrain->mesh.faces[out->ordinal], out->points, buffer);
    }
    return 1;
}


int bfm_forest_terrain_draw(BfmForestTerrain *terrain, const BfmForestCamera *camera,
                            uint32_t pixels[240][320])
{
    return bfm_forest_terrain_draw_actor(terrain,camera,NULL,NULL,NULL,0,pixels);
}
