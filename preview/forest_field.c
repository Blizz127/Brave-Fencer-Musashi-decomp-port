/* SPDX-License-Identifier: MIT */
#include "forest_field.h"
#include "forest_opening_data.h"
#include "forest_message_art.h"
#include "forest_floor.h"
#include "forest_hud.h"
#include "scene_payload.h"

#include <math.h>
#include <stdlib.h>

enum {
    FOREST_ROOM = 0x3008,
    FOREST_MEMBER = 77,
    FOREST_ENTRANCES = 2,
    SCENE_LIMIT = 1 << 20,
    GRID_HEADER = 32
};

/* Scene-owned tables selected by the ordinary forest room and door loaders. */
static const uint32_t scene_base = UINT32_C(0x80128158);
static const uint32_t room_descriptor_slot = UINT32_C(0x80186ad0);

typedef struct {
    uint16_t position[3];
    int16_t yaw;
    uint32_t on_enter, message;
} ForestEntrance;

struct BfmForestField {
    FloorGrid grid;
    BfmForestBlade blade;
    BfmForestOpeningData opening;
    uint8_t *scene;
    size_t scene_size;
    BfmForestTerrain *terrain;
    ForestEntrance entrances[FOREST_ENTRANCES];
    MusashiForestPlayerMesh *player;
};

static uint16_t read16(const uint8_t *p)
{
    return (uint16_t)(p[0] | (unsigned)p[1] << 8);
}

static uint32_t read32(const uint8_t *p)
{
    return read16(p) | (uint32_t)read16(p + 2) << 16;
}

/* Promote relative, little-endian floor faces into the variable-length
 * records used by the matched player-floor code. Every plane index is
 * checked before the matching routine may use it. */
static int load_faces(uint8_t **out, const uint8_t *src, size_t count,
                      unsigned planes, size_t normals, size_t distances)
{
    unsigned stride = 2 + 4 * planes;
    uint8_t *storage = malloc(count * stride);
    if (count && !storage)
        return 0;
    *out = storage;
    for (size_t i = 0; i < count; ++i, src += stride) {
        FloorFace *face = (FloorFace *)(storage + i * stride);
        face->flags = read16(src);
        for (unsigned j = 0; j < planes; ++j) {
            unsigned v = read16(src + 2 + j * 4);
            unsigned d = read16(src + 4 + j * 4);
            if (v >= normals || d >= distances || v > INT16_MAX || d > INT16_MAX)
                return 0;
            face->e[j].v = (s16)v;
            face->e[j].d = (s16)d;
        }
    }
    return 1;
}

static const uint8_t *scene_record(const uint8_t *scene, size_t size,
                                   uint32_t address, size_t length)
{
    if (address < scene_base || ((address - scene_base) & 3) || length > size ||
        address - scene_base > size - length)
        return NULL;
    return scene + (address - scene_base);
}

const uint8_t *bfm_forest_room_bytes(const BfmForestField *field, uint32_t address,
                                     size_t length)
{
    return field ? scene_record(field->scene, field->scene_size, address, length) : NULL;
}

BfmForestMessageArt *bfm_forest_open_message_art(const BfmForestField *field,
                                                 const MusashiDiscMedia *disc)
{
    return field ? bfm_forest_message_art_load(disc, field->scene, field->scene_size,
                                               field->terrain) : NULL;
}

BfmForestHud *bfm_forest_open_hud(const BfmForestField *field, const MusashiDiscMedia *disc)
{
    if (!field) return NULL;
    return bfm_forest_hud_load(disc, field->scene, field->scene_size, field->terrain);
}

static int load_entrances(BfmForestField *field, const uint8_t *scene, size_t size,
                          uint32_t table_address)
{
    const uint8_t *table = scene_record(scene, size, table_address, FOREST_ENTRANCES * 4);
    if (!table)
        return 0;
    for (unsigned i = 0; i < FOREST_ENTRANCES; ++i) {
        const uint8_t *record = scene_record(scene, size, read32(table + i * 4), 0x18);
        if (!record || read32(record) != FOREST_ROOM + i)
            return 0;
        for (unsigned axis = 0; axis < 3; ++axis)
            field->entrances[i].position[axis] = read16(record + 0x10 + axis * 2);
        field->entrances[i].on_enter = read32(record + 8);
        field->entrances[i].message = read32(record + 12);
        field->entrances[i].yaw = (int16_t)read16(record + 0x16);
    }
    return 1;
}

/* The ordinary field entrance installs its staged camera and scripted walk.
 * The initial view track and local velocity come from the mounted scene. */
static void load_opening_data(BfmForestField *field, const uint8_t *scene, size_t bytes,
                              const uint8_t *descriptor)
{
    if (field->entrances[0].on_enter != UINT32_C(0x8017dcb0))
        return;
    const uint32_t first_track = UINT32_C(0x801b1dac);
    const uint8_t *key = scene_record(scene, bytes, first_track, 12);
    const uint8_t *motion = scene_record(scene, bytes, UINT32_C(0x8018912c), 12);
    if (!key || !motion || read32(motion) || read32(motion + 4))
        return;
    uint32_t speed = read32(motion + 8);
    if (speed < (uint32_t)-0xb0000 || speed > (uint32_t)-0x70000)
        return;
    field->opening.first_track = first_track;
    field->opening.message = field->entrances[0].message;
    field->opening.initial_velocity = -(int32_t)(0u - speed);
    field->opening.animation_table = read32(descriptor + 0x4c);
    for (unsigned i = 0; i < 6; ++i)
        field->opening.key[i / 3][i % 3] = (int16_t)read16(key + i * 2);
}

int bfm_forest_opening_data(const BfmForestField *field, BfmForestOpeningData *out)
{
    if (!field || !out || !field->opening.first_track)
        return 0;
    *out = field->opening;
    return 1;
}

static int load_grid(FloorGrid *grid, const uint8_t *scene, size_t size,
                     const uint8_t *descriptor)
{
    const uint8_t *record = scene_record(scene, size, read32(descriptor + 0x88), GRID_HEADER);
    if (!record)
        return 0;
    size_t extent = size - (size_t)(record - scene);
    uint32_t off[6];
    for (unsigned i = 0; i < 6; ++i) {
        off[i] = read32(record + 8 + 4 * i);
        if (off[i] < GRID_HEADER || off[i] > extent || (off[i] & 1) ||
            (i && off[i] < off[i - 1]))
            return 0;
    }
    grid->x0 = read16(record);
    grid->z0 = read16(record + 2);
    grid->w = read16(record + 4);
    grid->h = read16(record + 6);
    size_t cells = (size_t)grid->w * grid->h;
    size_t distances = (off[2] - off[1]) / 4;
    size_t normals = (off[3] - off[2]) / 8;
    size_t ids = (off[4] - off[3]) / 2;
    size_t shorts = (off[5] - off[4]) / 18;
    if (!cells || cells > UINT16_MAX / 2 || off[1] - off[0] != cells * 4 ||
        (off[2] - off[1]) % 4 || (off[3] - off[2]) % 8 ||
        (off[5] - off[4]) % 18 || !distances || !normals || !ids)
        return 0;
    grid->cells = malloc(cells * 2 * sizeof(*grid->cells));
    grid->dist = malloc(distances * sizeof(*grid->dist));
    grid->verts = malloc(normals * sizeof(*grid->verts));
    uint16_t *entries = malloc(ids * sizeof(*entries));
    grid->ids = (uint8_t *)entries;
    if (!grid->cells || !grid->dist || !grid->verts || !entries)
        return 0;
    for (size_t i = 0; i < distances; ++i)
        grid->dist[i] = (s32)read32(record + off[1] + i * 4);
    for (size_t i = 0; i < normals; ++i) {
        const uint8_t *v = record + off[2] + i * 8;
        grid->verts[i] = (FloorPoint){read16(v), read16(v + 2), read16(v + 4),
                                      (s16)read16(v + 6)};
    }
    for (size_t i = 0; i < ids; ++i)
        entries[i] = read16(record + off[3] + i * 2);
    size_t longs = 0;
    for (size_t i = 0; i < cells; ++i) {
        const uint8_t *c = record + off[0] + i * 4;
        unsigned begin = read16(c), count = read16(c + 2);
        /* Empty cells carry a sentinel offset. Store a safe, unused origin. */
        grid->cells[i * 2] = count ? (u16)begin : 0;
        grid->cells[i * 2 + 1] = (u16)count;
        if (count && ((begin & 1) || begin / 2 > ids || count > ids - begin / 2))
            return 0;
        for (unsigned j = 0; j < count; ++j) {
            unsigned id = entries[begin / 2 + j];
            if (id & 0x8000) {
                size_t end = (id & 0x7fff) + 1;
                if (end > longs)
                    longs = end;
            } else if (id >= shorts)
                return 0;
        }
    }
    return longs <= (extent - off[5]) / 22 &&
           load_faces(&grid->faces18, record + off[4], shorts, 4, normals, distances) &&
           load_faces(&grid->faces22, record + off[5], longs, 5, normals, distances);
}

void bfm_forest_field_free(BfmForestField *field)
{
    if (field) {
        bfm_forest_terrain_free(field->terrain);
        free(field->scene);
        free(field->grid.cells);
        free(field->grid.dist);
        free(field->grid.verts);
        free(field->grid.ids);
        free(field->grid.faces18);
        free(field->grid.faces22);
        musashi_forest_player_mesh_free(field->player);
        free(field);
    }
}

BfmForestField *bfm_forest_field_open(const MusashiDiscMedia *disc, uint16_t room)
{
    if (!disc || room != FOREST_ROOM)
        return NULL;
    size_t bytes;
    uint8_t *scene = bfm_scene_unpack_pac0(disc, "SC01.CD", FOREST_MEMBER,
                                           SCENE_LIMIT, SCENE_LIMIT, &bytes);
    if (!scene)
        return NULL;
    BfmForestField *field = calloc(1, sizeof(*field));
    const uint8_t *slot = scene_record(scene, bytes, room_descriptor_slot, 4);
    const uint8_t *descriptor = slot ? scene_record(scene, bytes, read32(slot), 0xcc) : NULL;
    if (field && descriptor && bfm_forest_blade_load(&field->blade, scene, bytes) &&
        load_entrances(field, scene, bytes, read32(descriptor + 0xc8)) &&
        load_grid(&field->grid, scene, bytes, descriptor)) {
        load_opening_data(field, scene, bytes, descriptor);
        const uint8_t *mesh = scene_record(scene, bytes, read32(descriptor + 0x94), 8);
        const uint8_t *end = scene_record(scene, bytes, read32(descriptor + 0x88), GRID_HEADER);
        if (mesh && end && end > mesh)
            field->terrain = bfm_forest_terrain_load(disc, mesh, (size_t)(end - mesh));
        if (field->terrain && bfm_forest_terrain_set_lighting(field->terrain, descriptor, 0x98))
            field->player = musashi_forest_player_mesh_load(disc);
    }
    if (field) {
        field->scene = scene;
        field->scene_size = bytes;
    } else {
        free(scene);
    }
    if (field && !field->player) {
        bfm_forest_field_free(field);
        field = NULL;
    }
    return field;
}

int bfm_forest_draw_ground(BfmForestField *field, const BfmForestCamera *camera,
                           uint32_t pixels[240][320])
{
    return field && bfm_forest_terrain_draw(field->terrain, camera, pixels);
}

int bfm_forest_draw_player(BfmForestField *field, const BfmForestCamera *camera,
                           const BfmForestPose *pose, const BfmForestTransform *root,
                           uint32_t visible_parts, uint32_t pixels[240][320])
{
    return bfm_forest_draw_player_buffer(field, camera, pose, root, visible_parts, 0, pixels);
}

int bfm_forest_draw_player_buffer(BfmForestField *field, const BfmForestCamera *camera,
                           const BfmForestPose *pose, const BfmForestTransform *root,
                           uint32_t visible_parts, unsigned buffer, uint32_t pixels[240][320])
{
    return field && pose && root && bfm_forest_terrain_draw_actor_buffer(field->terrain, camera,
                         field->player, pose, root, visible_parts, buffer, pixels);
}

int bfm_forest_draw_entrance_player(BfmForestField *field, const BfmForestCamera *camera,
                           const BfmForestPose *pose, const BfmForestTransform *root,
                           uint32_t visible_parts, unsigned buffer, const BfmForestBladeFrame *frame,
                           uint32_t pixels[240][320])
{
    return field && pose && root && bfm_forest_terrain_draw_entrance(field->terrain, camera,
               field->player, pose, root, visible_parts, &field->blade, buffer, frame, pixels);
}

const MusashiForestPlayerPart *bfm_forest_player_part(const BfmForestField *field,
                                                       unsigned part)
{
    return field ? musashi_forest_player_mesh_part(field->player, part) : NULL;
}

int bfm_forest_place_player(const BfmForestField *field, unsigned entrance,
                            ForestPlayerState *player, int16_t *model_yaw,
                            int16_t cached_position[3])
{
    if (!field || entrance >= FOREST_ENTRANCES || !player || !model_yaw || !cached_position)
        return 0;
    const ForestEntrance *door = &field->entrances[entrance];
    for (unsigned axis = 0; axis < 3; ++axis) {
        uint16_t coordinate = door->position[axis];
        player->position[axis] = (player->position[axis] & UINT32_C(0xffff)) |
                                 ((uint32_t)coordinate << 16);
        player->position_88[axis] = player->position_90[axis] = (int16_t)coordinate;
        player->position_154[axis] = player->position_15c[axis] = (int16_t)coordinate;
        cached_position[axis] = (int16_t)coordinate;
    }
    *model_yaw = door->yaw;
    return 1;
}

/* Scratch registers referenced by the original four native floor routines. */
static FloorPoint probe[3];
FloorPoint *D_801870AC = &probe[0];
FloorPoint *D_801870B0 = &probe[1];
FloorPoint *D_801870B4 = &probe[2];
FloorPoint D_801152A8;
FloorGrid *D_801D94F0;
u16 D_801D9500;

int bfm_forest_ground(const BfmForestField *field, int16_t x, int16_t y,
                      int16_t z, BfmForestContact *contact)
{
    if (!field || !contact)
        return 0;
    FloorPoint point = {(u16)x, (u16)y, (u16)z, 0};
    D_801D94F0 = (FloorGrid *)&field->grid;
    u32 slope = func_80134510(&point);
    D_801D94F0 = NULL;
    if (!slope)
        return 0;
    *contact = (BfmForestContact){(int16_t)point.y,
        {(int16_t)D_801152A8.x, (int16_t)D_801152A8.y, (int16_t)D_801152A8.z},
        D_801D9500, (uint16_t)slope};
    return 1;
}

typedef struct {
    const u16 *tokens;
    unsigned count;
} ContactCell;

static ContactCell contact_cell(const FloorGrid *grid, const int16_t p[3])
{
    unsigned col = (p[0] + 32768) / 128, row = (p[2] + 32768) / 128;
    if (col < grid->x0 || col - grid->x0 >= grid->w ||
        row < grid->z0 || row - grid->z0 >= grid->h)
        return (ContactCell){0};
    const u16 *cell = grid->cells + 2 * ((row - grid->z0) * grid->w + col - grid->x0);
    return (ContactCell){(const u16 *)(grid->ids + cell[0]), cell[1]};
}

static const FloorFace *contact_face(const FloorGrid *grid, u16 token)
{
    unsigned index = token & 0x7fff;
    return (const FloorFace *)((token & 0x8000) ?
        grid->faces22 + 22 * index : grid->faces18 + 18 * index);
}

static int64_t signed_distance(const FloorGrid *grid, FloorPlane plane, const int16_t p[3])
{
    const FloorPoint *n = &grid->verts[plane.v];
    return (int64_t)(s16)n->x * p[0] + (int64_t)(s16)n->y * p[1] +
           (int64_t)(s16)n->z * p[2] + grid->dist[plane.d];
}

static int same_grid_index(int16_t a, int16_t b)
{
    return (a + 32768) / 128 == (b + 32768) / 128;
}

static int flat_floor(const FloorFace *face, const FloorPoint *n)
{
    return !face->flags && !n->x && !n->z && (s16)n->y < 0;
}

int bfm_forest_clear_flat_step(const BfmForestField *field, const int16_t from[3],
                               int16_t x, int16_t z)
{
    if (!field || !from || !same_grid_index(from[0], x) || !same_grid_index(from[2], z))
        return 0;
    const FloorGrid *grid = &field->grid;
    const int16_t to[3] = {x, from[1], z};
    ContactCell cell = contact_cell(grid, from);
    int have_floor = 0;
    for (unsigned i = 0; i < cell.count; ++i) {
        const FloorFace *face = contact_face(grid, cell.tokens[i]);
        const FloorPoint *normal = &grid->verts[face->e[0].v];
        if (flat_floor(face, normal)) {
            have_floor = 1;
            continue;
        }
        if (face->flags != 0x100 || normal->y ||
            signed_distance(grid, face->e[0], from) <= 4 * 4096 ||
            signed_distance(grid, face->e[0], to) <= 4 * 4096)
            return 0;
    }
    return have_floor;
}

enum { EDGE_TOLERANCE = 0x2f00 };

/* A cell seam can pass through a single floor polygon. Keep the route on
 * the actual flat polygons indexed by the original doorway cell. */
static int on_doorway_floor(const FloorGrid *grid, ContactCell doorway, const int16_t p[3])
{
    for (unsigned i = 0; i < doorway.count; ++i) {
        u16 token = doorway.tokens[i];
        const FloorFace *face = contact_face(grid, token);
        const FloorPoint *n = &grid->verts[face->e[0].v];
        if (!flat_floor(face, n) ||
            llabs(signed_distance(grid, face->e[0], p)) > 4 * 4096)
            continue;
        unsigned sides = (token & 0x8000) ? 4 : 3;
        unsigned j = 1;
        while (j <= sides && signed_distance(grid, face->e[j], p) >= -EDGE_TOLERANCE)
            ++j;
        if (j > sides)
            return 1;
    }
    return 0;
}

/* The traced corridor consists of the entrance floor, a one-in-four rise
 * along Z, and a level landing. The registered downward probe chooses the
 * actual bounded floor in the current cell. */
static int corridor_ground(const BfmForestContact *ground)
{
    return !ground->flags && ground->slope == 0x2000 && !ground->normal[0] &&
           ground->normal[1] < 0 &&
           (!ground->normal[2] || ground->normal[1] == 4 * ground->normal[2]);
}

/* Test the mounted polygon at the integer point where the motion segment
 * enters its plane. Face edge planes use the same Q12 tolerance. */
static int64_t crossing_margin(const FloorGrid *grid, const FloorFace *face, unsigned sides,
                               const int16_t from[3], const int16_t to[3])
{
    int64_t start = signed_distance(grid, face->e[0], from);
    int64_t end = signed_distance(grid, face->e[0], to);
    if (start < 0 || end >= 0)
        return INT64_MIN;
    int16_t hit[3];
    for (unsigned i = 0; i < 3; ++i)
        hit[i] = (int16_t)(from[i] + (to[i] - from[i]) * start / (start - end));
    int64_t margin = INT64_MAX;
    for (unsigned j = 1; j <= sides; ++j) {
        int64_t distance = signed_distance(grid, face->e[j], hit);
        if (distance < margin)
            margin = distance;
    }
    return margin;
}

/* The normal is Q12. Round the projected penetration and the outward
 * clearance separately, retaining the game's whole-unit contact point. */
static int slide_wall(const FloorGrid *grid, FloorPlane plane, int16_t end[3])
{
    const FloorPoint *n = &grid->verts[plane.v];
    int nx = (s16)n->x, nz = (s16)n->z;
    int64_t length_sq = (int64_t)nx * nx + (int64_t)nz * nz;
    if (!length_sq)
        return 0;
    int64_t depth = -signed_distance(grid, plane, end);
    int64_t x = end[0] + depth * nx / length_sq;
    int64_t z = end[2] + depth * nz / length_sq;
    int64_t distance = x * nx + z * nz + grid->dist[plane.d];
    /* Rounding reduces the outward displacement. Start at the first
     * whole step that could achieve the required separation. */
    int64_t step = 3;
    if (distance < EDGE_TOLERANCE) {
        int64_t required = ((EDGE_TOLERANCE - distance) * 4096 + length_sq - 1) / length_sq;
        if (required > step)
            step = required;
    }
    for (;; ++step) {
        int64_t px = x + step * nx / 4096;
        int64_t pz = z + step * nz / 4096;
        if (px < INT16_MIN || px > INT16_MAX || pz < INT16_MIN || pz > INT16_MAX)
            return 0;
        if (px * nx + pz * nz + grid->dist[plane.d] >= EDGE_TOLERANCE) {
            end[0] = (int16_t)px;
            end[2] = (int16_t)pz;
            return 1;
        }
    }
}

/* A correction can carry the endpoint into a second wall. Revisit the
 * mounted faces in their stored order after each correction. */
static int resolve_cell(const FloorGrid *grid, ContactCell cell, const int16_t from[3],
                        int16_t end[3], int airborne)
{
    if (!cell.count)
        return -1;
    for (unsigned i = 0; i < cell.count; ++i) {
        u16 token = cell.tokens[i];
        const FloorFace *face = contact_face(grid, token);
        const FloorPoint *n = &grid->verts[face->e[0].v];
        if ((s16)n->y < 0 && !(face->flags & 0x300))
            continue;
        unsigned sides = (token & 0x8000) ? 4 : 3;
        if ((face->flags != 0x100 && face->flags != 0x2000) || n->y || (!n->x && !n->z)) {
            if (airborne && crossing_margin(grid, face, sides, from, end) < -EDGE_TOLERANCE)
                continue;
            return -1;
        }
        if (crossing_margin(grid, face, sides, from, end) >= -EDGE_TOLERANCE)
            return slide_wall(grid, face->e[0], end) ? 1 : -1;
    }
    return 0;
}

static int resolve_walls(const FloorGrid *grid, ContactCell start,
                         const int16_t from[3], int16_t end[3], int airborne)
{
    for (unsigned pass = 0; pass < 6; ++pass) {
        int hit = resolve_cell(grid, start, from, end, airborne);
        if (hit == 0 && (!same_grid_index(from[0], end[0]) || !same_grid_index(from[2], end[2])))
            hit = resolve_cell(grid, contact_cell(grid, end), from, end, airborne);
        if (hit <= 0)
            return hit == 0;
    }
    return 0;
}

/* Adjacent polygons on the ramp share a plane. Recover the actual support
 * under the actor; another floor in the same cell may face the same way. */
static const FloorPlane *support_plane(const FloorGrid *grid, ContactCell cell,
                                       const BfmForestContact *ground, const int16_t from[3])
{
    if (ground->height > INT16_MAX - 2)
        return NULL;
    int16_t hit[3] = {from[0], (int16_t)(ground->height + 2), from[2]};
    for (unsigned i = cell.count; i; --i) {
        u16 token = cell.tokens[i - 1];
        const FloorFace *face = contact_face(grid, token);
        const FloorPoint *n = &grid->verts[face->e[0].v];
        if (face->flags != ground->flags || (s16)n->x != ground->normal[0] ||
            (s16)n->y != ground->normal[1] || (s16)n->z != ground->normal[2] ||
            llabs(signed_distance(grid, face->e[0], hit)) > -(s16)n->y)
            continue;
        unsigned sides = (token & 0x8000) ? 4 : 3;
        unsigned j = 1;
        while (j <= sides && signed_distance(grid, face->e[j], hit) >= -EDGE_TOLERANCE)
            ++j;
        if (j > sides)
            return &face->e[0];
    }
    return NULL;
}

/* Sample the terrain grid toward the lower integer coordinate on either side
 * of zero. The divisor must be positive. */
static int64_t grid_floor(int64_t numerator, int64_t denominator)
{
    int64_t quotient = numerator / denominator;
    return quotient - (numerator % denominator < 0);
}

/* The terrain sweep lowers its far endpoint by the horizontal step length.
 * Its projection supplies the grid sample for the actor base. The lateral
 * correction removes only whole units of penetration. */
static int follow_corridor_floor(const FloorGrid *grid, ContactCell cell,
                                 const BfmForestContact *ground,
                                 const int16_t from[3], int16_t end[3])
{
    if (!ground->normal[2]) {
        end[1] = ground->height;
        return 1;
    }
    const FloorPlane *plane = support_plane(grid, cell, ground, from);
    if (!plane)
        return 0;
    int dx = end[0] - from[0], dz = end[2] - from[2];
    int span = (int)sqrt((double)(dx * dx + dz * dz));
    if (span < 3)
        span = 4;
    else if (span < 5)
        ++span;
    int ny = ground->normal[1], nz = ground->normal[2];
    int64_t bias = grid->dist[plane->d];
    int64_t distance = (int64_t)ny * (end[1] + span + 1) + (int64_t)nz * end[2] + bias;
    int64_t length_sq = (int64_t)ny * ny + (int64_t)nz * nz;
    int64_t penetration_z = nz * distance;
    int64_t sample_z = end[2] + grid_floor(-penetration_z, length_sq);
    int64_t y = grid_floor((int64_t)nz * sample_z + bias, -ny) - 2;
    int64_t z = end[2] - penetration_z / length_sq;
    if (y < INT16_MIN || y > INT16_MAX || z < INT16_MIN || z > INT16_MAX)
        return 0;
    end[1] = (int16_t)y;
    end[2] = (int16_t)z;
    return 1;
}

/* Both sides of the raised obstacle carry the rim's material. Only their
 * measured, upward-facing inclines support this walking contact. */
static int raised_normal(unsigned flags, int nx, int ny, int nz)
{
    return flags == 0x2000 && ny == -3825 && (abs(nx) == 280 || abs(nx) == 283) &&
           abs(nz) == 1434 && (nx < 0) == (nz < 0);
}

/* Prefer a polygon containing the crossing over its neighbor whose edge
 * tolerance overlaps the same point at the crest. */
static const FloorPlane *raised_support(const FloorGrid *grid, ContactCell cell,
                                        const int16_t near[3], const int16_t far[3])
{
    const FloorPlane *edge = NULL;
    for (unsigned i = 0; i < cell.count; ++i) {
        u16 token = cell.tokens[i];
        const FloorFace *face = contact_face(grid, token);
        const FloorPoint *v = &grid->verts[face->e[0].v];
        if (!raised_normal(face->flags, (s16)v->x, (s16)v->y, (s16)v->z))
            continue;
        unsigned sides = (token & 0x8000) ? 4 : 3;
        int64_t margin = crossing_margin(grid, face, sides, near, far);
        if (margin >= 0)
            return &face->e[0];
        if (!edge && margin >= -EDGE_TOLERANCE)
            edge = &face->e[0];
    }
    return edge;
}

BfmForestTopStep bfm_forest_raised_step(const BfmForestField *field,
    const BfmForestContact *previous, const int16_t from[3], int16_t to[3])
{
    if (!field || !previous || !from || !to || from[1] != to[1] ||
        abs(from[1] - previous->height) > 4 || previous->slope != 0x2000 ||
        !raised_normal(previous->flags, previous->normal[0], previous->normal[1], previous->normal[2]))
        return BFM_FOREST_TOP_BLOCKED;
    int dx = to[0] - from[0], dz = to[2] - from[2];
    if (abs(dx) > 16 || abs(dz) > 16 ||
        (!same_grid_index(from[0], to[0]) && !same_grid_index(from[2], to[2])))
        return BFM_FOREST_TOP_BLOCKED;
    if (!dx && !dz)
        return BFM_FOREST_TOP_GROUNDED;
    const FloorGrid *grid = &field->grid;
    ContactCell start = contact_cell(grid, from);
    int16_t far[3] = {to[0], to[1], to[2]};
    if (!resolve_walls(grid, start, from, far, 0))
        return BFM_FOREST_TOP_BLOCKED;
    int span = (int)sqrt((double)(dx * dx + dz * dz));
    if (from[1] < INT16_MIN + 4 || far[1] > INT16_MAX - span - 1)
        return BFM_FOREST_TOP_BLOCKED;
    int16_t near[3] = {from[0], (int16_t)(from[1] - 4), from[2]};
    far[1] += span + 1;
    const FloorPlane *plane = raised_support(grid, start, near, far);
    if (!plane)
        plane = raised_support(grid, contact_cell(grid, to), near, far);
    if (!plane)
        return BFM_FOREST_TOP_BLOCKED;

    /* The foot sweeps to the far point below the proposed step. Project onto
     * the contacted surface. Resolve lateral penetration and clearance as
     * separate integer steps, with two units above the rounded plane height. */
    const FloorPoint *v = &grid->verts[plane->v];
    int nx = (s16)v->x, ny = (s16)v->y, nz = (s16)v->z;
    int64_t norm = nx * nx + ny * ny + nz * nz;
    int64_t depth = -signed_distance(grid, *plane, far);
    int64_t x = far[0] + nx * depth / norm + nx * 3 / 4096;
    int64_t y = far[1] + grid_floor(2 * ny * depth + norm, 2 * norm) - 2;
    int64_t z = far[2] + nz * depth / norm + nz * 3 / 4096;
    if (x < INT16_MIN || x > INT16_MAX || y < INT16_MIN || y > INT16_MAX ||
        z < INT16_MIN || z > INT16_MAX)
        return BFM_FOREST_TOP_BLOCKED;
    BfmForestContact ground;
    if (!bfm_forest_ground(field, (int16_t)x, (int16_t)y, (int16_t)z, &ground))
        return BFM_FOREST_TOP_BLOCKED;
    BfmForestTopStep result;
    if (raised_normal(ground.flags, ground.normal[0], ground.normal[1], ground.normal[2]) &&
        abs(ground.height - (int)y) <= 4)
        result = BFM_FOREST_TOP_GROUNDED;
    else if (corridor_ground(&ground) && !ground.normal[2] && ground.height > y + 4)
        result = BFM_FOREST_TOP_LEFT;
    else
        return BFM_FOREST_TOP_BLOCKED;
    to[0] = (int16_t)x;
    to[1] = (int16_t)y;
    to[2] = (int16_t)z;
    return result;
}

int bfm_forest_corridor_step(const BfmForestField *field, const BfmForestContact *previous,
                             const int16_t from[3], int16_t to[3])
{
    if (!field || !previous || !from || !to || from[1] != to[1] ||
        abs(to[0] - from[0]) > 16 || abs(to[2] - from[2]) > 16)
        return 0;
    const FloorGrid *grid = &field->grid;
    ContactCell start = contact_cell(grid, from);
    int same_x = same_grid_index(from[0], to[0]), same_z = same_grid_index(from[2], to[2]);
    if (!same_x && !same_z)
        return 0;
    int16_t clipped[3] = {to[0], to[1], to[2]};
    BfmForestContact ground;
    if (!corridor_ground(previous) || abs(previous->height - from[1]) > 4 ||
        !resolve_walls(grid, start, from, clipped, 0) ||
        !follow_corridor_floor(grid, start, previous, from, clipped) ||
        !bfm_forest_ground(field, clipped[0], clipped[1], clipped[2], &ground) ||
        !corridor_ground(&ground) || abs(ground.height - clipped[1]) > 4)
        return 0;
    to[0] = clipped[0];
    to[1] = clipped[1];
    to[2] = clipped[2];
    return 1;
}

int bfm_forest_doorway_step(const BfmForestField *field, const int16_t origin[3],
                            const int16_t from[3], int16_t to[3])
{
    if (!field || !origin || !from || !to || origin[1] != from[1])
        return 0;
    ContactCell doorway = contact_cell(&field->grid, origin);
    BfmForestContact ground;
    int16_t end[3] = {to[0], to[1], to[2]};
    if (!on_doorway_floor(&field->grid, doorway, from) ||
        !bfm_forest_ground(field, from[0], from[1], from[2], &ground) ||
        !bfm_forest_corridor_step(field, &ground, from, end) ||
        end[1] != from[1] || !on_doorway_floor(&field->grid, doorway, end))
        return 0;
    to[0] = end[0];
    to[2] = end[2];
    return 1;
}

/* Sweep the actor base while it is above the entrance floor. The ordinary
 * downward probe starts above the proposed contact so a thin raised surface
 * is not lost after the feet pass through it. */
int bfm_forest_air_step(const BfmForestField *field, const int16_t from[3],
                        int16_t to[3], BfmForestContact *contact, int *landed)
{
    if (!field || !from || !to || !contact || !landed ||
        abs(to[0] - from[0]) > 16 || abs(to[2] - from[2]) > 16 ||
        abs(to[1] - from[1]) > 32)
        return 0;
    const FloorGrid *grid = &field->grid;
    int16_t end[3] = {to[0], to[1], to[2]};
    if (!resolve_walls(grid, contact_cell(grid, from), from, end, 1))
        return 0;
    BfmForestContact ground;
    int16_t upper = end[1] < from[1] ? end[1] : from[1];
    if (!bfm_forest_ground(field, end[0], upper, end[2], &ground) ||
        ground.slope != 0x2000 || (ground.flags && ground.flags != 0x2000))
        return 0;
    int hit = end[1] >= from[1] && end[1] > ground.height + 2;
    if (hit) {
        int64_t nx = ground.normal[0], ny = ground.normal[1], nz = ground.normal[2];
        if (nx || nz) {
            const FloorPlane *plane = support_plane(grid, contact_cell(grid, end), &ground, end);
            if (!plane)
                return 0;
            int64_t length = nx * nx + ny * ny + nz * nz;
            int64_t distance = signed_distance(grid, *plane, end) + 2 * ny;
            end[0] -= (int16_t)(nx * distance / length);
            end[2] -= (int16_t)(nz * distance / length);
            if (!bfm_forest_ground(field, end[0], upper, end[2], &ground))
                return 0;
        }
        end[1] = ground.height;
    }
    for (unsigned i = 0; i < 3; ++i)
        to[i] = end[i];
    *contact = ground;
    *landed = hit;
    return 1;
}
