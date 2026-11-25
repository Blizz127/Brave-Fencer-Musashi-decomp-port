/* SPDX-License-Identifier: MIT */
#include "forest_field.h"
#include "forest_opening_data.h"
#include "forest_message_art.h"
#include "forest_floor.h"
#include "forest_hud.h"
#include "scene_payload.h"

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

/* Intersect the segment with the actual bounded wall polygon. Signed edge
 * distances interpolate linearly, so no division or rounded point is needed. */
static int crosses_face(const FloorGrid *grid, const FloorFace *face, unsigned sides,
                        const int16_t from[3], const int16_t to[3])
{
    int64_t start = signed_distance(grid, face->e[0], from);
    int64_t end = signed_distance(grid, face->e[0], to);
    if (start < 0 || end >= 0)
        return 0;
    for (unsigned j = 1; j <= sides; ++j) {
        int64_t a = signed_distance(grid, face->e[j], from) + EDGE_TOLERANCE;
        int64_t b = signed_distance(grid, face->e[j], to) + EDGE_TOLERANCE;
        if (a * -end + b * start < 0)
            return 0;
    }
    return 1;
}

static int resolve_cell(const FloorGrid *grid, ContactCell cell, int oblique,
                        const int16_t from[3], const int16_t to[3], int16_t clipped[3])
{
    if (!cell.count)
        return 0;
    for (unsigned i = 0; i < cell.count; ++i) {
        u16 token = cell.tokens[i];
        const FloorFace *face = contact_face(grid, token);
        const FloorPoint *n = &grid->verts[face->e[0].v];
        if (flat_floor(face, n))
            continue;
        if (face->flags != 0x100 || n->y || (!n->x && !n->z))
            return 0;
        if ((n->x && n->z) != oblique ||
            !crosses_face(grid, face, (token & 0x8000) ? 4 : 3, from, to))
            continue;
        if (oblique)
            return 0;
        unsigned axis = n->x ? 0 : 2;
        int coefficient = n->x ? (s16)n->x : (s16)n->z;
        int64_t bias = grid->dist[face->e[0].d];
        if (coefficient != 4095 || bias % coefficient)
            return 0;
        /* The two measured perpendicular doorway walls hold the integer
         * actor origin three world units inside their mounted planes. */
        int64_t stop = -bias / coefficient + 3;
        if (stop < INT16_MIN || stop > INT16_MAX)
            return 0;
        if (clipped[axis] < stop)
            clipped[axis] = (int16_t)stop;
    }
    return 1;
}

int bfm_forest_doorway_step(const BfmForestField *field, const int16_t origin[3],
                            const int16_t from[3], int16_t to[3])
{
    if (!field || !origin || !from || !to || origin[1] != from[1] || from[1] != to[1] ||
        abs(to[0] - from[0]) > 16 || abs(to[2] - from[2]) > 16)
        return 0;
    const FloorGrid *grid = &field->grid;
    ContactCell doorway = contact_cell(grid, origin), start = contact_cell(grid, from);
    ContactCell end = contact_cell(grid, to);
    int same_x = same_grid_index(from[0], to[0]), same_z = same_grid_index(from[2], to[2]);
    if (!same_x && !same_z)
        return 0;
    int16_t clipped[3] = {to[0], to[1], to[2]};
    if (!on_doorway_floor(grid, doorway, from) ||
        !resolve_cell(grid, start, 0, from, to, clipped) ||
        ((!same_x || !same_z) && !resolve_cell(grid, end, 0, from, to, clipped)))
        return 0;
    end = contact_cell(grid, clipped);
    if (!resolve_cell(grid, start, 1, from, clipped, clipped) ||
        ((!same_grid_index(from[0], clipped[0]) || !same_grid_index(from[2], clipped[2])) &&
         !resolve_cell(grid, end, 1, from, clipped, clipped)) ||
        !on_doorway_floor(grid, doorway, clipped))
        return 0;
    to[0] = clipped[0];
    to[2] = clipped[2];
    return 1;
}
