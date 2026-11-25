/* SPDX-License-Identifier: MIT */
#include "forest_field.h"
#include "forest_opening_data.h"
#include "forest_message_art.h"
#include "forest_floor.h"
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

int bfm_forest_clear_flat_step(const BfmForestField *field, const int16_t from[3],
                               int16_t x, int16_t z)
{
    if (!field || !from)
        return 0;
    const FloorGrid *grid = &field->grid;
    unsigned cx = (x + 32768) / 128, cz = (z + 32768) / 128;
    if (cx != (unsigned)(from[0] + 32768) / 128 ||
        cz != (unsigned)(from[2] + 32768) / 128 ||
        cx < grid->x0 || cx - grid->x0 >= grid->w ||
        cz < grid->z0 || cz - grid->z0 >= grid->h)
        return 0;
    size_t slot = (cz - grid->z0) * grid->w + cx - grid->x0;
    const u16 *cell = grid->cells + 2 * slot;
    const u16 *tokens = (const u16 *)(grid->ids + cell[0]);
    int have_floor = 0;
    for (unsigned i = 0; i < cell[1]; ++i) {
        unsigned token = tokens[i], id = token & 0x7fff;
        const FloorFace *face = (const FloorFace *)((token & 0x8000) ?
            grid->faces22 + 22 * id : grid->faces18 + 18 * id);
        const FloorPoint *n = &grid->verts[face->e[0].v];
        if (!face->flags && !n->x && !n->z && (s16)n->y < 0) {
            have_floor = 1;
            continue;
        }
        if (face->flags != 0x100 || n->y)
            return 0;
        /* Only the clear side is qualified. A small approach margin leaves
         * wall contact and sliding to the full field collision routine. */
        int64_t bias = grid->dist[face->e[0].d];
        int64_t nx = (s16)n->x, nz = (s16)n->z;
        int64_t margin = 4 * 4096;
        if (nx * from[0] + nz * from[2] + bias <= margin ||
            nx * x + nz * z + bias <= margin)
            return 0;
    }
    return have_floor;
}
