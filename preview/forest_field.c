/* SPDX-License-Identifier: MIT */
#include "forest_field.h"
#include "room_walk_grid.h"
#include "tower_ascent_floor.h"
#include "forest_entry.h"
#include "forest_bridge.h"
#include "forest_upper_trunks.h"
#include "forest_rifle_wall.h"
#include "forest_shore.h"
#include "forest_boulders.h"
#include "forest_drift.h"
#include "forest_glow_halo.h"
#include "forest_glow_figures.h"
#include "forest_rifles.h"
#include "forest_rifle_round.h"
#include "forest_rifle_contact.h"
#include "forest_muzzle_fan.h"
#include "forest_fall.h"
#include "forest_water_effect.h"
#include "forest_shore_motion.h"
#include "forest_guard_contact.h"
#include "forest_lesson_guard.h"
#include "forest_fusion_model.h"
#include "forest_muzzle_art.h"
#include "forest_shot_effects.h"
#include "forest_lesson_fusion.h"
#include "forest_tether_trails.h"
#include "forest_trail.h"
#include "forest_shore_figures.h"
#include "forest_lift.h"
#include "forest_strike_effects.h"
#include "forest_sweep.h"
#include "forest_opening_data.h"
#include "forest_message_art.h"
#include "forest_floor.h"
#include "forest_hud.h"
#include "scene_payload.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

enum {
    FOREST_ROOM = 0x3008,
    FOREST_MEMBER = 77,
    TOWER_ROOM = 0x3009,
    TOWER_MEMBER = 80,
    TOWER_INTERIOR_ROOM = 0x300a,
    TOWER_INTERIOR_MEMBER = 84,
    SUMMIT_ROOM = 0x300b,
    SUMMIT_MEMBER = 0,
    DRAWBRIDGE_ROOM = 0x300d,
    DRAWBRIDGE_MEMBER = 4,
    FOREST_ENTRANCES = 2,
    SCENE_LIMIT = 1 << 20,
    GRID_HEADER = 32
};

/* Scene-owned tables selected by the ordinary forest room and door loaders. */
static const uint32_t scene_base = UINT32_C(0x80128158);
static const uint32_t forest_descriptor_slot = UINT32_C(0x80186ad0);
static const uint32_t tower_descriptor_slot = UINT32_C(0x8018350c);
static const uint32_t tower_interior_descriptor_slot = UINT32_C(0x80186800);
static const uint32_t summit_descriptor_slot = UINT32_C(0x8018abb0);
static const uint32_t drawbridge_descriptor_slot = UINT32_C(0x8017dc90);

typedef struct {
    uint16_t position[3];
    int16_t yaw;
    uint32_t address, on_enter, message;
} ForestEntrance;

typedef struct {
    const uint8_t *record;
    uint16_t entry;
    int16_t root[3], low[3], high[3];
    const FloorFace *face;
    int active, issued;
} ForestGateway;

enum { FOREST_MELEE_GUARDS = 2 };

/* The original scanout trails these submitted field actors and effects by two
 * updates. Their logical collision and animation states continue immediately. */
typedef struct {
    struct {
        BfmForestTransform root;
        BfmForestPose pose;
        uint32_t parts;
    } guard[FOREST_MELEE_GUARDS];
    BfmForestActor fusion_actor;
    BfmForestTransform fusion_root;
    BfmForestCamera camera;
    BfmForestDustCard cards[1 + BFM_FOREST_NOTICE_CARDS + BFM_FOREST_STRIKE_CARDS + BFM_FOREST_SWEEP_CARDS +
                            BFM_FOREST_WATER_EFFECT_CARDS];
    unsigned count;
    BfmForestDustCard skill_cards[BFM_TETHER_CARDS + BFM_FOREST_MUZZLE_CARDS +
                                BFM_GUNSHOT_FAN_CARDS + BFM_FOREST_SHOT_EFFECT_CARDS];
    size_t skill_count;
} BfmForestSubmittedFrame;

/* A live round spans 100 updates. Even a re-entering sentry needs 19
 * updates to reach the firing key, allowing six outstanding rounds per rifle. */
enum {
    FOREST_BANK_ROUNDS = 6 * BFM_FOREST_BANK_RIFLES,
    FOREST_BLADE_TARGETS = FOREST_MELEE_GUARDS + BFM_FOREST_BANK_RIFLES + BFM_FOREST_SHORE_FIGURES,
    FOREST_BANK_BODIES = 4 + FOREST_MELEE_GUARDS + 2 * BFM_FOREST_SHORE_FIGURES,
    FOREST_ATMOSPHERE_BODIES = BFM_FOREST_GLOWS + BFM_FOREST_BANK_RIFLES + FOREST_BANK_ROUNDS +
                               BFM_FOREST_BOULDERS,
    FOREST_FIELD_BODIES = FOREST_BANK_BODIES + FOREST_ATMOSPHERE_BODIES +
                          BFM_FOREST_BRIDGE_TARGET_COUNT + BFM_FOREST_UPPER_TRUNKS
};
_Static_assert(FOREST_FIELD_BODIES <= BFM_FOREST_ACTOR_LIMIT,
               "The field renderer must accommodate the submitted actor pool");

typedef struct {
    unsigned count;
    BfmForestDustCard cards[BFM_FOREST_HALO_CARDS + BFM_FOREST_FALL_SLOTS + BFM_FOREST_BANK_FLASH_CARDS];
    BfmForestActor bodies[FOREST_ATMOSPHERE_BODIES];
    BfmForestTransform roots[FOREST_ATMOSPHERE_BODIES];
    BfmForestPose rifle_poses[BFM_FOREST_BANK_RIFLES];
    BfmForestCamera camera;
    unsigned body_count;
} BfmForestAtmosphereFrame;

typedef struct {
    enum { SHORE_SOLDIER, RIFLE_SOLDIER, SHORE_FIGURE } kind;
    unsigned placement;
} ForestBladeTarget;

struct BfmForestField {
    uint16_t room;
    FloorGrid grid;
    int tower_floor_raised;
    ForestGateway gateway;
    BfmForestBridge bridge;
    BfmForestUpperTrunks *upper_trunks;
    const FloorPlane *first_descent;
    int bank_start_z, bank_end_z, bank_height;
    unsigned far_bank_cells;
    BfmForestBlade blade;
    BfmForestDust dust;
    BfmForestWaterEffect water_effect;
    BfmForestDrift drift;
    BfmForestGlows lights;
    BfmForestGlowFigures *light_figures;
    BfmForestRifles *rifles;
    BfmForestRifleRoundArt *round_art;
    BfmForestRifleRound rounds[FOREST_BANK_ROUNDS];
    unsigned player_hit_power;
    int16_t player_hit_yaw;
    BfmForestMuzzleFan flashes[BFM_FOREST_BANK_RIFLES];
    BfmForestFall fall;
    BfmForestAtmosphereFrame atmosphere[3];
    unsigned next_atmosphere;
    BfmForestShore *shore, *bank_patrol;
    BfmForestLessonGuard *lesson_guard;
    BfmFusionModel *fusion_model;
    BfmForestTetherTrails tether_trails;
    BfmForestMuzzleArt muzzle;
    BfmForestBulletArt flare;
    BfmForestShotEffects shot_effects;
    const BfmForestActor *fusion_actor;
    BfmForestGuardBox guard_box;
    BfmForestBoulders boulders;
    ForestBladeTarget targets[FOREST_BLADE_TARGETS];
    int first_shore_hit;
    BfmForestTrail *trail;
    BfmForestShoreFigures *shore_figures;
    BfmForestStrikeEffects strikes;
    BfmForestSweep sweep;
    BfmForestSubmittedFrame submitted[3];
    unsigned next_frame;
    BfmForestOpeningData opening;
    uint8_t *scene;
    size_t scene_size;
    BfmForestTerrain *terrain, *summit_road;
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

BfmForestShoreFigures *bfm_forest_lift_figures(BfmForestField *field)
{
    return field ? field->shore_figures : NULL;
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
        /* The summit's second record leads to the escape, beyond room 300C. */
        unsigned destination = i && field->room == SUMMIT_ROOM ? 0x300d : field->room + i;
        if (!record || read32(record) != destination)
            return 0;
        for (unsigned axis = 0; axis < 3; ++axis)
            field->entrances[i].position[axis] = read16(record + 0x10 + axis * 2);
        field->entrances[i].address = read32(table + i * 4);
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
                     uint32_t address)
{
    const uint8_t *record = scene_record(scene, size, address, GRID_HEADER);
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

struct BfmRoomWalkGrid {
    FloorGrid grid;
};

static void free_grid(FloorGrid *grid)
{
    free(grid->cells);
    free(grid->dist);
    free(grid->verts);
    free(grid->ids);
    free(grid->faces18);
    free(grid->faces22);
}

int bfm_tower_ascent_floor_activate(BfmForestField *field)
{
    if (!field || field->room != TOWER_ROOM) return 0;
    if (field->tower_floor_raised) return 1;
    const uint8_t *slot = scene_record(field->scene, field->scene_size, tower_descriptor_slot, 4);
    const uint8_t *room = slot ? scene_record(field->scene, field->scene_size, read32(slot), 0x90) : NULL;
    /* The risen courtyard has its own cell directory and textured surfaces.
     * In the US room member it ends where the first figure begins. */
    const uint32_t raised_mesh = UINT32_C(0x80193c64);
    const size_t mesh_bytes = UINT32_C(0x801a176c) - raised_mesh;
    const uint8_t *mesh = scene_record(field->scene, field->scene_size, raised_mesh, mesh_bytes);
    FloorGrid raised = {0};
    if (!room || !mesh ||
        !load_grid(&raised, field->scene, field->scene_size, read32(room + 0x8c)) ||
        !bfm_forest_terrain_replace_mesh(field->terrain, mesh, mesh_bytes)) {
        free_grid(&raised);
        return 0;
    }
    free_grid(&field->grid);
    field->grid = raised;
    field->tower_floor_raised = 1;
    return 1;
}

BfmRoomWalkGrid *bfm_room_walk_grid_open(const uint8_t *scene, size_t bytes, uint32_t room)
{
    const uint8_t *descriptor = scene ? scene_record(scene, bytes, room, 0x8c) : NULL;
    BfmRoomWalkGrid *walk = descriptor ? calloc(1, sizeof *walk) : NULL;
    if (walk && !load_grid(&walk->grid, scene, bytes, read32(descriptor + 0x88))) {
        bfm_room_walk_grid_free(walk);
        return NULL;
    }
    return walk;
}

BfmForestField *bfm_forest_motion_surface_open(const uint8_t *scene, size_t bytes, uint32_t room)
{
    const uint8_t *descriptor = scene ? scene_record(scene, bytes, room, 0x8c) : NULL;
    if (!descriptor) return NULL;
    BfmForestField *surface = calloc(1, sizeof *surface);
    if (surface && !load_grid(&surface->grid, scene, bytes, read32(descriptor + 0x88))) {
        bfm_forest_field_free(surface);
        return NULL;
    }
    return surface;
}

void bfm_room_walk_grid_free(BfmRoomWalkGrid *walk)
{
    if (walk) free_grid(&walk->grid);
    free(walk);
}

BfmForestBoulders *bfm_forest_room_boulders(BfmForestField *field)
{
    return &field->boulders;
}

int bfm_forest_melee_brace(const BfmForestField *field, const int16_t from[3])
{
    const BfmForestActor *guards[FOREST_MELEE_GUARDS] = {
        bfm_forest_shore_actor(field->shore), bfm_forest_shore_actor(field->bank_patrol)
    };
    int16_t center[3];
    for (unsigned i = 0; i < FOREST_MELEE_GUARDS; ++i)
        if (guards[i] && bfm_forest_guard_contact(&field->guard_box, guards[i]->root, from, from, center))
            return 1;
    return 0;
}

static int blocks_boulder(const BfmForestGuardBox *box, const BfmForestActor *actor,
                         const int16_t from[3], const int16_t to[3])
{
    if (!actor) return 0;
    const int16_t at[3] = {(int16_t)actor->root->translation[0],
                           (int16_t)actor->root->translation[1],
                           (int16_t)actor->root->translation[2]};
    return bfm_forest_boulder_obstruction(box, at, from, to);
}

int bfm_forest_boulder_path_blocked(BfmForestField *field, unsigned skip,
                                    const int16_t from[3], const int16_t tip[3])
{
    for (unsigned i = 0; i < BFM_FOREST_BOULDERS; ++i) {
        BfmForestBoulder *other = &field->boulders.placed[i];
        if (i != skip && other->active &&
            bfm_forest_boulder_obstruction(&field->boulders.box, other->at, from, tip))
            return 1;
    }
    if (bfm_forest_shore_figures_block_rock(field->shore_figures, from, tip)) return 1;
    const BfmForestActor *guards[FOREST_MELEE_GUARDS] = {
        bfm_forest_shore_actor(field->shore), bfm_forest_shore_actor(field->bank_patrol)
    };
    for (unsigned i = 0; i < FOREST_MELEE_GUARDS; ++i)
        if (blocks_boulder(&field->guard_box, guards[i], from, tip))
            return 1;
    for (unsigned i = 0; i < BFM_FOREST_BANK_RIFLES; ++i) {
        const BfmForestActor *rifle = bfm_forest_rifle_actor(field->rifles, i);
        if (blocks_boulder(&field->guard_box, rifle, from, tip))
            return 1;
    }
    return 0;
}

void bfm_forest_field_free(BfmForestField *field)
{
    if (field) {
        bfm_forest_barrier_art_free(field->boulders.art);
        bfm_forest_rifle_round_art_free(field->round_art);
        bfm_forest_rifles_free(field->rifles);
        bfm_forest_glow_figures_free(field->light_figures);
        bfm_forest_upper_trunks_free(field->upper_trunks);
        bfm_forest_bridge_targets_free(field->bridge.targets);
        bfm_forest_shore_figures_free(field->shore_figures);
        bfm_forest_trail_free(field->trail);
        bfm_forest_lesson_guard_free(field->lesson_guard);
        bfm_fusion_model_free(field->fusion_model);
        bfm_forest_shore_free(field->bank_patrol);
        bfm_forest_shore_free(field->shore);
        bfm_forest_terrain_free(field->summit_road);
        bfm_forest_terrain_free(field->terrain);
        free(field->scene);
        free_grid(&field->grid);
        musashi_forest_player_mesh_free(field->player);
        free(field);
    }
}

static void load_entrance_descent(BfmForestField *field);
static void load_gateway(BfmForestField *field, const uint8_t *descriptor);

/* These placements register in room-list order along the first bank. */
static int load_blade_targets(BfmForestField *field, const uint8_t *descriptor)
{
    const unsigned entries = 12;
    uint32_t list = read32(descriptor + 0xb8);
    const uint8_t *records = bfm_forest_room_bytes(field, list, entries * 20);
    if (!records) return 0;
    unsigned figures = 0, rifles = 0, shore = 0, count = 0;
    for (unsigned i = 0; i < entries; ++i) {
        const uint8_t *row = records + 20 * i;
        unsigned kind = (uint16_t)read16(row + 6), variant = (uint16_t)read16(row + 8);
        ForestBladeTarget target;
        if (kind == 12 && variant == 0) {
            if (figures == BFM_FOREST_SHORE_FIGURES) return 0;
            target = (ForestBladeTarget){SHORE_FIGURE, figures++};
        } else if (kind == 47 && variant == 0) {
            if (shore >= FOREST_MELEE_GUARDS) return 0;
            if (shore == 1) {
                field->bank_patrol = bfm_forest_shore_open_patrol(field->shore, row);
                if (!field->bank_patrol) return 0;
            }
            target = (ForestBladeTarget){SHORE_SOLDIER, shore++};
        } else if (kind == 47 && variant == 0x500 + rifles && rifles < BFM_FOREST_BANK_RIFLES) {
            target = (ForestBladeTarget){RIFLE_SOLDIER, rifles++};
        } else continue;
        field->targets[count++] = target;
    }
    return count == FOREST_BLADE_TARGETS && shore == FOREST_MELEE_GUARDS;
}

BfmForestField *bfm_forest_field_open(const MusashiDiscMedia *disc, uint16_t room)
{
    if (!disc || (room != FOREST_ROOM && room != TOWER_ROOM && room != TOWER_INTERIOR_ROOM &&
                  room != SUMMIT_ROOM && room != DRAWBRIDGE_ROOM))
        return NULL;
    unsigned member = room == TOWER_ROOM ? TOWER_MEMBER : FOREST_MEMBER;
    uint32_t descriptor_slot = room == TOWER_ROOM ? tower_descriptor_slot : forest_descriptor_slot;
    const char *archive = "SC01.CD";
    if (room == SUMMIT_ROOM) {
        archive = "SC02.CD";
        member = SUMMIT_MEMBER;
        descriptor_slot = summit_descriptor_slot;
    } else if (room == DRAWBRIDGE_ROOM) {
        archive = "SC02.CD";
        member = DRAWBRIDGE_MEMBER;
        descriptor_slot = drawbridge_descriptor_slot;
    } else if (room == TOWER_INTERIOR_ROOM) {
        member = TOWER_INTERIOR_MEMBER;
        descriptor_slot = tower_interior_descriptor_slot;
    }
    size_t bytes;
    uint8_t *scene = bfm_scene_unpack_pac0(disc, archive, member,
                                           SCENE_LIMIT, SCENE_LIMIT, &bytes);
    if (!scene)
        return NULL;
    BfmForestField *field = calloc(1, sizeof(*field));
    if (!field) {
        free(scene);
        return NULL;
    }
    field->room = room;
    field->scene = scene;
    field->scene_size = bytes;
    const uint8_t *slot = scene_record(scene, bytes, descriptor_slot, 4);
    const uint8_t *descriptor = slot ? scene_record(scene, bytes, read32(slot), 0xcc) : NULL;
    if (descriptor && load_entrances(field, scene, bytes, read32(descriptor + 0xc8)) &&
        load_grid(&field->grid, scene, bytes, read32(descriptor + 0x88))) {
        const uint8_t *mesh = scene_record(scene, bytes, read32(descriptor + 0x94), 8);
        /* The drawbridge floor precedes its mesh. Its panorama map follows
         * the mesh and bounds the final occupied terrain cell. */
        const uint8_t *end = room == DRAWBRIDGE_ROOM
            ? scene_record(scene, bytes, read32(descriptor + 0x30), 8)
            : scene_record(scene, bytes, read32(descriptor + 0x88), GRID_HEADER);
        if (mesh && end && end > mesh)
            field->terrain = bfm_forest_terrain_load_archive(disc, archive, member, mesh,
                                                           (size_t)(end - mesh));
        if (field->terrain && bfm_forest_terrain_set_lighting(field->terrain, descriptor, 0x98))
            field->player = musashi_forest_player_mesh_load(disc);
    }
    if (!field->player) {
        bfm_forest_field_free(field);
        return NULL;
    }
    if (room == DRAWBRIDGE_ROOM) return field;
    if (room == TOWER_INTERIOR_ROOM || room == SUMMIT_ROOM) {
        size_t outline = room == SUMMIT_ROOM ? 0x659e0 : 0x61630;
        if (bfm_castle_blade_load(&field->blade, scene, bytes, outline) &&
            (room == SUMMIT_ROOM || bfm_forest_dust_load(&field->dust, disc)))
            return field;
        bfm_forest_field_free(field);
        return NULL;
    }
    if (room == TOWER_ROOM) {
        if (bfm_tower_blade_load(&field->blade, scene, bytes))
            return field;
        bfm_forest_field_free(field);
        return NULL;
    }

    load_entrance_descent(field);
    load_opening_data(field, scene, bytes, descriptor);
    if (bfm_forest_blade_load(&field->blade, scene, bytes) &&
        bfm_forest_drift_load(&field->drift, field, read32(descriptor + 0xb4)) &&
        bfm_forest_glows_load(&field->lights, field, descriptor) &&
        bfm_forest_fall_load(&field->fall, field, read32(descriptor + 0xb4)))
        field->shore = bfm_forest_shore_open(field, scene, bytes, scene_base, descriptor);
    if (field->shore && bfm_forest_guard_box_load(field, &field->guard_box) &&
        load_blade_targets(field, descriptor) &&
        bfm_forest_boulders_load(&field->boulders, field, read32(descriptor + 0xb8))) {
        load_gateway(field, descriptor);
        field->shore_figures = bfm_forest_shore_figures_open(field);
        field->light_figures = bfm_forest_glow_figures_open(field);
        field->rifles = bfm_forest_rifles_open(field);
        field->round_art = bfm_forest_rifle_round_art_open(field);
        field->trail = bfm_forest_trail_open(field, field->shore);
        field->upper_trunks = bfm_forest_upper_trunks_open(field);
    }
    if (!field->shore_figures || !field->trail || !field->upper_trunks || !field->light_figures ||
        !field->rifles || !field->round_art ||
        !bfm_forest_bridge_load(&field->bridge, field) ||
        !bfm_forest_water_effect_load(&field->water_effect, field) ||
        !bfm_forest_dust_load(&field->dust, disc)) {
        bfm_forest_field_free(field);
        return NULL;
    }
    return field;
}

const uint8_t *bfm_forest_field_scene(const BfmForestField *field, size_t *size)
{
    if (!field || !size) return NULL;
    *size = field->scene_size;
    return field->scene;
}

const uint16_t (*bfm_forest_field_tiles(const BfmForestField *field))[1024]
{
    return field ? bfm_forest_terrain_tiles(field->terrain) : NULL;
}

const uint8_t *bfm_forest_field_entrance(const BfmForestField *field, unsigned index)
{
    return field && index < FOREST_ENTRANCES
        ? bfm_forest_room_bytes(field, field->entrances[index].address, 36) : NULL;
}

int bfm_forest_mount_summit_road(BfmForestField *field, const MusashiDiscMedia *disc)
{
    if (!field || field->room != SUMMIT_ROOM || !disc) return 0;
    if (field->summit_road) return 1;
    const uint8_t *slot = bfm_forest_room_bytes(field, summit_descriptor_slot, 4);
    const uint8_t *room = slot ? bfm_forest_room_bytes(field, read32(slot), 0xcc) : NULL;
    if (!room) return 0;
    const uint8_t *mesh = bfm_forest_room_bytes(field, read32(room + 0x98), 8);
    const uint8_t *end = bfm_forest_room_bytes(field, read32(room + 0x88), GRID_HEADER);
    if (!mesh || !end || end <= mesh) return 0;
    BfmForestTerrain *road = bfm_forest_terrain_load_archive(disc, "SC02.CD", SUMMIT_MEMBER,
                                                             mesh, (size_t)(end - mesh));
    if (!road || !bfm_forest_terrain_set_lighting(road, room, 0x98)) {
        bfm_forest_terrain_free(road);
        return 0;
    }
    field->summit_road = road;
    return 1;
}

int bfm_forest_draw_room(const BfmForestField *field, const BfmForestCamera *camera,
                         const BfmForestPose *pose, const BfmForestTransform *root,
                         uint32_t visible_parts, unsigned buffer, const BfmForestBladeFrame *blade,
                         const BfmForestActor *actors, size_t actor_count, const BfmForestDustCard *cards, size_t card_count,
                         uint32_t pixels[240][320])
{
    return field && bfm_forest_terrain_draw_entrance_with_effects(
        field->summit_road ? field->summit_road : field->terrain, camera,
        field->player, pose, root, visible_parts, &field->blade, buffer, blade,
        actors, actor_count, cards, card_count, pixels);
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

int bfm_forest_draw_player_only(const BfmForestField *field, const BfmForestCamera *camera,
                                const BfmForestPose *pose, const BfmForestTransform *root,
                                uint32_t mask, const BfmForestModelLighting *lighting, uint32_t pixels[240][320])
{
    return field && bfm_forest_terrain_draw_player_only(field->terrain, camera, field->player,
                                                         pose, root, mask, lighting, pixels);
}

int bfm_forest_draw_player_buffer(BfmForestField *field, const BfmForestCamera *camera,
                           const BfmForestPose *pose, const BfmForestTransform *root,
                           uint32_t visible_parts, unsigned buffer, uint32_t pixels[240][320])
{
    return field && pose && root && bfm_forest_terrain_draw_actor_buffer(field->terrain, camera,
                         field->player, pose, root, visible_parts, buffer, pixels);
}

void bfm_forest_start_ambient(BfmForestField *field, BfmGameplayRun *run)
{
    bfm_forest_drift_start(&field->drift, run);
    bfm_forest_glows_start(&field->lights, run);
}

void bfm_forest_submit_atmosphere(BfmForestField *field, const BfmForestCamera *camera,
                                  const int32_t eye[3])
{
    BfmForestAtmosphereFrame *frame = &field->atmosphere[field->next_atmosphere];
    frame->camera = *camera;
    frame->body_count = bfm_forest_glow_figures_collect(field->light_figures, &field->lights, frame->bodies);
    for (unsigned i = 0; i < BFM_FOREST_BANK_RIFLES; ++i) {
        const BfmForestActor *rifle = bfm_forest_rifle_actor(field->rifles, i);
        if (rifle) {
            BfmForestActor *body = &frame->bodies[frame->body_count++];
            *body = *rifle;
            frame->rifle_poses[i] = *rifle->pose;
            body->pose = &frame->rifle_poses[i];
        }
    }
    for (unsigned i = 0; i < FOREST_BANK_ROUNDS; ++i) {
        BfmForestActor *body = &frame->bodies[frame->body_count];
        /* Round poses are immutable room art. The frame retains the selected
         * key here and saves the moving root and camera below. */
        if (bfm_forest_rifle_round_actor(&field->rounds[i], field->round_art, body))
            ++frame->body_count;
    }
    frame->body_count += bfm_forest_boulders_draw(&field->boulders, frame->bodies + frame->body_count);
    for (unsigned i = 0; i < frame->body_count; ++i) {
        frame->roots[i] = *frame->bodies[i].root;
        frame->bodies[i].root = &frame->roots[i];
        frame->bodies[i].submitted_camera = &frame->camera;
    }
    frame->count = bfm_forest_glows_project(&field->lights, camera, frame->cards);
    frame->count += bfm_forest_fall_project(&field->fall, camera, frame->cards + frame->count,
                                           BFM_FOREST_FALL_SLOTS);
    for (unsigned i = 0; i < BFM_FOREST_BANK_RIFLES; ++i) {
        unsigned light;
        const BfmForestActor *flash = bfm_forest_rifle_flash(field->rifles, i, &light);
        if (!flash) continue;
        if (light == 128) {
            int16_t muzzle[3];
            for (unsigned axis = 0; axis < 3; ++axis)
                muzzle[axis] = (int16_t)flash->root->translation[axis];
            if (!bfm_forest_muzzle_fan_start(&field->flashes[i], field, muzzle, eye)) continue;
        }
        frame->count += bfm_forest_muzzle_fan_project(&field->flashes[i], camera, light,
                                                      frame->cards + frame->count);
    }
    field->next_atmosphere = (field->next_atmosphere + 1) % 3;
}

void bfm_forest_tick_dust(BfmForestField *field, BfmForestDustEmission event,
                          const int16_t player[3], const int32_t camera_eye[3], int yaw)
{
    bfm_forest_dust_tick(&field->dust, event, player, camera_eye, yaw);
}

/* Apply the field model's signed fixed-point transform to a collision point. */
static int strike_world_point(const BfmForestTransform *root, const int local[3],
                              int16_t world[3])
{
    for (unsigned row = 0; row < 3; ++row) {
        int sum = 0;
        for (unsigned axis = 0; axis < 3; ++axis)
            sum += root->rotation[row][axis] * local[axis];
        int64_t at = (int64_t)root->translation[row] + sum / 4096 - (sum % 4096 < 0);
        if (at < INT16_MIN || at > INT16_MAX) return 0;
        world[row] = (int16_t)at;
    }
    return 1;
}

/* For the first-strike preview the shore soldier is the sole target. */
static int contact_target(BfmForestField *field, const ForestBladeTarget *target,
                          const int16_t start[3], const int16_t end[3], uint16_t yaw,
                          const uint16_t *strength, uint16_t type, int16_t center[3],
                          int16_t point[3], int16_t normal[3])
{
    if (!strength && (target->kind != SHORE_SOLDIER || target->placement)) return 0;
    if (target->kind == SHORE_FIGURE)
        return bfm_forest_shore_figure_contact(field->shore_figures, target->placement, start, end,
                                               type, *strength, (int16_t)yaw, center, point, normal);
    BfmForestShore *soldier = target->placement ? field->bank_patrol : field->shore;
    const BfmForestActor *actor = target->kind == SHORE_SOLDIER
        ? bfm_forest_shore_actor(soldier) : bfm_forest_rifle_actor(field->rifles, target->placement);
    if (!actor || !bfm_forest_guard_contact_point(&field->guard_box, actor->root,
                                                  start, end, center, point, normal)) return 0;
    if (!strength) {
        bfm_forest_shore_recoil(field->shore, yaw);
        field->first_shore_hit = 1;
    } else if (target->kind == SHORE_SOLDIER) {
        if (!bfm_forest_shore_take_typed_hit(soldier, yaw, *strength, type)) return 0;
    } else if (!bfm_forest_rifle_take_hit(field->rifles, target->placement, yaw, *strength, type)) {
        return 0;
    }
    bfm_forest_dust_recoil_start(&field->dust, actor->root->translation, yaw);
    return 1;
}

size_t bfm_forest_blade_rays(const BfmForestTransform *root, const int8_t tip[3],
                             const int8_t next_tip[3], int16_t start[3],
                             int16_t ends[BFM_FOREST_STRIKE_RAYS][3])
{
    const int origin[3] = {0, -30, 0};
    if (!root || !tip || !next_tip || !start || !ends || !strike_world_point(root, origin, start))
        return 0;
    int points[3][3], squared = 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        int delta = (int)next_tip[axis] - tip[axis];
        squared += delta * delta;
        points[0][axis] = tip[axis];
        points[1][axis] = tip[axis] + delta / 3;
        points[2][axis] = next_tip[axis] - delta / 3;
    }
    /* The field adds two probes when its quantized distance reaches 65.
     * Across signed-byte endpoints the first qualifying squared length is 4288. */
    unsigned probes = squared >= 4288 ? 3 : 1;
    size_t valid = 0;
    for (unsigned i = 0; i < probes; ++i)
        if (strike_world_point(root, points[i], ends[valid])) ++valid;
    return valid;
}

static size_t scan_ground_strike(BfmForestField *field, const BfmForestTransform *player_root,
                                const int8_t tip[3], const int8_t next_tip[3], int16_t *yaw,
                                BfmForestHit hits[BFM_FOREST_STRIKE_RAYS],
                                const uint16_t *strength, uint16_t attack_type)
{
    if (!field || !player_root || !tip || !next_tip || !yaw || !hits || (!strength && field->first_shore_hit))
        return 0;
    int16_t start[3], ends[BFM_FOREST_STRIKE_RAYS][3], center[3];
    size_t probes = bfm_forest_blade_rays(player_root, tip, next_tip, start, ends);
    unsigned struck = 0;
    uint16_t incoming_yaw = (uint16_t)*yaw;
    size_t hit_count = 0;
    for (unsigned i = 0; i < probes; ++i) {
        BfmForestHit ray = {0};
        int16_t *end = ends[i];
        for (unsigned j = 0; j < FOREST_BLADE_TARGETS; ++j) {
            if (struck & (1u << j)) continue;
            int16_t position[3], normal[3];
            int hit = contact_target(field, &field->targets[j], start, end, incoming_yaw,
                                     strength, attack_type, center, position, normal);
            if (!hit) continue;
            memcpy(end, position, sizeof position);
            if (hit == 4) {
                if (ray.connected) memcpy(ray.position, position, sizeof position);
                continue;
            }
            struck |= 1u << j;
            *yaw = (int16_t)bfm_forest_shore_bearing(player_root->translation[0] - center[0],
                                                     player_root->translation[2] - center[2]);
            ray.connected = 1;
            if (strength) ray.strength = *strength;
            memcpy(ray.position, position, sizeof position);
            memcpy(ray.normal, normal, sizeof normal);
        }
        if (ray.connected) hits[hit_count++] = ray;
    }
    return hit_count;
}

static int resolve_ground_strike(BfmForestField *field, const BfmForestTransform *player_root,
                                 const int8_t tip[3], const int8_t next_tip[3], int16_t *yaw,
                                 int16_t effect_position[3], int16_t effect_normal[3],
                                 const uint16_t *strength, uint16_t attack_type)
{
    BfmForestHit hits[BFM_FOREST_STRIKE_RAYS];
    size_t count = scan_ground_strike(field, player_root, tip, next_tip, yaw, hits, strength, attack_type);
    if (!count) return 0;
    const BfmForestHit *last = &hits[count - 1];
    if (effect_position) memcpy(effect_position, last->position, sizeof last->position);
    if (effect_normal) memcpy(effect_normal, last->normal, sizeof last->normal);
    return 1;
}

size_t bfm_forest_ground_strike_rays(BfmForestField *field, const BfmForestTransform *player_root,
                                    const int8_t tip[3], const int8_t next_tip[3],
                                    uint16_t strength, uint16_t attack_type, int16_t *yaw,
                                    BfmForestHit hits[BFM_FOREST_STRIKE_RAYS])
{
    if (attack_type != 1 && attack_type != 2) return 0;
    return scan_ground_strike(field, player_root, tip, next_tip, yaw, hits, &strength, attack_type);
}

int bfm_forest_first_strike(BfmForestField *field, const BfmForestTransform *player_root,
                            const int8_t tip[3], const int8_t next_tip[3], int16_t *yaw,
                            int16_t effect_position[3], int16_t effect_normal[3])
{
    return resolve_ground_strike(field, player_root, tip, next_tip, yaw,
                                effect_position, effect_normal, NULL, 0);
}

int bfm_forest_ground_strike(BfmForestField *field, const BfmForestTransform *player_root,
                             const int8_t tip[3], const int8_t next_tip[3], uint16_t strength,
                             int16_t *yaw, int16_t effect_position[3], int16_t effect_normal[3])
{
    return resolve_ground_strike(field, player_root, tip, next_tip, yaw,
                                effect_position, effect_normal, &strength, 1);
}

int bfm_forest_typed_ground_strike(BfmForestField *field, const BfmForestTransform *player_root,
                                  const int8_t tip[3], const int8_t next_tip[3],
                                  uint16_t strength, uint16_t attack_type, int16_t *yaw,
                                  int16_t effect_position[3], int16_t effect_normal[3])
{
    if (attack_type != 1 && attack_type != 2) return 0;
    return resolve_ground_strike(field, player_root, tip, next_tip, yaw,
                                 effect_position, effect_normal, &strength, attack_type);
}

int bfm_forest_tick_sweep(BfmForestField *field, const MusashiDiscMedia *disc,
                          int begin, uint32_t logical_pose, const BfmForestTransform *previous_root)
{
    if (begin) return bfm_forest_sweep_begin(&field->sweep, field);
    return bfm_forest_sweep_step(&field->sweep, disc, logical_pose, previous_root);
}

int bfm_forest_queue_hurt_number(BfmForestField *field, const int16_t player[3], uint16_t amount)
{
    return field && player && bfm_forest_strike_effects_queue_hurt(&field->strikes, field, player, amount);
}

void bfm_forest_begin_water_effect(BfmForestField *field, const int16_t foot[3],
                                   const int32_t eye[3], BfmGameplayRun *run)
{
    if (field && field->room == FOREST_ROOM)
        bfm_forest_water_effect_begin(&field->water_effect, foot, eye, run);
}

int bfm_forest_submit_guard_and_impact(BfmForestField *field, const BfmForestHit *hit,
                           const int32_t camera_eye[3], const BfmForestCamera *camera)
{
    return bfm_forest_submit_guard_and_impact_with_run(field, hit, camera_eye, camera, NULL);
}

int bfm_forest_submit_guard_and_impact_with_run(BfmForestField *field, const BfmForestHit *hit,
                           const int32_t camera_eye[3], const BfmForestCamera *camera,
                           BfmGameplayRun *run)
{
    return bfm_forest_submit_guard_and_impacts_with_run(field, hit,
                hit && hit->connected ? 1 : 0, camera_eye, camera, run);
}

int bfm_forest_submit_guard_and_impacts_with_run(BfmForestField *field,
                           const BfmForestHit *hits, size_t count, const int32_t camera_eye[3],
                           const BfmForestCamera *camera, BfmGameplayRun *run)
{
    if (!field || !camera || !camera_eye || (count && !hits)) return 0;
    int connected = 0;
    for (size_t i = 0; i < count; ++i) {
        if (!hits[i].connected) continue;
        if (!bfm_forest_strike_effects_queue(&field->strikes, field, &hits[i], camera_eye)) return 0;
        connected = 1;
    }
    if (!connected) bfm_forest_dust_recoil_advance(&field->dust);
    BfmForestSubmittedFrame *frame = &field->submitted[field->next_frame];
    frame->camera = *camera;
    frame->count = bfm_forest_dust_recoil_project(&field->dust, camera, frame->cards);
    BfmForestShore *guards[FOREST_MELEE_GUARDS] = {field->shore, field->bank_patrol};
    for (unsigned i = 0; i < FOREST_MELEE_GUARDS; ++i) {
        if (bfm_forest_marker_project(bfm_forest_shore_marker(guards[i]), camera,
                                      &frame->cards[frame->count])) ++frame->count;
    }
    for (unsigned i = 0; i < BFM_FOREST_SHORE_FIGURES; ++i) {
        if (bfm_forest_marker_project(bfm_forest_shore_figure_marker(field->shore_figures, i),
                                      camera, &frame->cards[frame->count])) ++frame->count;
    }
    frame->count += bfm_forest_strike_effects_step(&field->strikes, camera_eye, camera, run,
                                                  frame->cards + frame->count);
    frame->count += bfm_forest_sweep_project(&field->sweep, camera, frame->cards + frame->count);
    frame->count += (unsigned)bfm_forest_water_effect_project(&field->water_effect, camera,
                                frame->cards + frame->count, BFM_FOREST_WATER_EFFECT_CARDS);
    bfm_forest_water_effect_tick(&field->water_effect);
    for (unsigned i = 0; i < FOREST_MELEE_GUARDS; ++i) {
        const BfmForestActor *guard = bfm_forest_shore_actor(guards[i]);
        frame->guard[i].parts = guard ? guard->visible_parts : 0;
        if (guard) {
            frame->guard[i].root = *guard->root;
            frame->guard[i].pose = *guard->pose;
        }
    }
    frame->skill_count = bfm_forest_tether_trails_project(&field->tether_trails, camera,
                                                          frame->skill_cards, BFM_TETHER_CARDS);
    frame->skill_count += bfm_forest_field_project_gunshots(field, camera,
        frame->skill_cards + frame->skill_count,
        sizeof frame->skill_cards / sizeof frame->skill_cards[0] - frame->skill_count);
    frame->fusion_actor.visible_parts = 0;
    if (field->fusion_actor) {
        frame->fusion_root = *field->fusion_actor->root;
        frame->fusion_actor = *field->fusion_actor;
        frame->fusion_actor.root = &frame->fusion_root;
        frame->fusion_actor.submitted_camera = &frame->camera;
    }
    field->next_frame = (field->next_frame + 1) % 3;
    return 1;
}

size_t bfm_forest_field_project_gunshots(const BfmForestField *field, const BfmForestCamera *camera,
                                         BfmForestDustCard *cards, size_t capacity)
{
    size_t count = bfm_forest_muzzle_project(&field->muzzle, camera, cards, capacity);
    count += bfm_forest_bullet_art_project(&field->flare, camera, cards + count, capacity - count);
    return count + bfm_forest_shot_effects_project(&field->shot_effects, &field->dust,
                                                    camera, cards + count, capacity - count);
}

int bfm_forest_field_gunshot_art(BfmForestField *field, const BfmForestAward *award,
    const BfmForestTransform *root, const int32_t eye[3], BfmGameplayRun *run, int fired)
{
    bfm_forest_muzzle_tick(&field->muzzle);
    bfm_forest_bullet_art_tick(&field->flare);
    if (!fired) return 1;
    if (!field->muzzle.fan.loaded) {
        if (!bfm_forest_muzzle_load(&field->muzzle, award)) return 0;
        field->flare = field->muzzle.fan;
    }
    int16_t emitter[3];
    if (!bfm_forest_muzzle_fire(&field->muzzle, root, bfm_gameplay_random(run), emitter)) return 0;
    bfm_forest_bullet_art_fire(&field->flare, emitter, eye);
    return 1;
}

static unsigned trail_scatter(void *run)
{
    return bfm_gameplay_random(run);
}

void bfm_forest_field_advance_gunshots(BfmForestField *field, BfmForestBullets *bullets,
    const int32_t eye[3], BfmGameplayRun *run, BfmGunshotContact contact, void *context)
{
    BfmBulletEvent events[BFM_FOREST_BULLETS];
    bfm_forest_shot_effects_tick(&field->shot_effects, eye, trail_scatter, run);
    unsigned ability = run->player.record[0x49] & 0x7fu;
    size_t count = bfm_forest_bullets_tick(bullets, ability, contact, context, events);
    for (size_t i = 0; i < count; ++i) {
        const BfmBulletEvent *event = &events[i];
        if (event->kind == BFM_BULLET_ACTOR_HIT)
            bfm_forest_shot_effects_hit(&field->shot_effects, event->position, event->normal,
                                        eye, trail_scatter, run);
        else if (event->kind == BFM_BULLET_FADE)
            bfm_forest_shot_effects_expire(&field->shot_effects, event->position, eye);
    }
}

int bfm_forest_field_fusion_effects(BfmForestField *field, const MusashiDiscMedia *disc,
    const BfmFusionFlight *flight, const BfmForestTether *tether, int was_attached,
    int absorption_edge, const int16_t player[3], BfmGameplayRun *run)
{
    if (flight->phase == BFM_FUSION_OUTBOUND && !field->fusion_model) {
        field->fusion_model = bfm_fusion_model_open(disc);
        if (!field->fusion_model) return 0;
    }
    field->fusion_actor = bfm_fusion_model_tick(field->fusion_model, flight, absorption_edge);
    int attached = tether->phase == BFM_TETHER_ATTACHED && field->lesson_guard;
    int16_t blade[3];
    for (unsigned i = 0; i < 3; ++i) blade[i] = (int16_t)flight->model.translation[i];
    if (attached && !was_attached) {
        const BfmForestActor *guard = bfm_forest_lesson_guard_actor(field->lesson_guard);
        if (!guard) return 0;
        int16_t target[3];
        for (unsigned i = 0; i < 3; ++i) target[i] = (int16_t)guard->root->translation[i];
        bfm_forest_tether_trails_begin(&field->tether_trails, target, blade);
    } else if (field->tether_trails.enabled) {
        bfm_forest_tether_trails_tick(&field->tether_trails, attached, tether->meter,
            field->fusion_actor ? blade : NULL, player, trail_scatter, run);
    }
    return 1;
}

BfmForestLessonGuard *bfm_forest_field_lesson_guard(const BfmForestField *field)
{
    return field ? field->lesson_guard : NULL;
}

int bfm_forest_field_summon_gun_guard(BfmForestField *field)
{
    if (!field || field->lesson_guard) return 0;
    field->lesson_guard = bfm_forest_lesson_guard_spawn(field);
    return field->lesson_guard != NULL;
}

void bfm_forest_field_remove_gun_guard(BfmForestField *field)
{
    if (!field) return;
    bfm_forest_lesson_guard_free(field->lesson_guard);
    field->lesson_guard = NULL;
}

unsigned bfm_forest_field_bridge_step(BfmForestField *field, unsigned room_phase)
{
    return field ? bfm_forest_bridge_advance(&field->bridge, room_phase) : room_phase;
}

int bfm_forest_bridge_impact(void *context, const int16_t from[3], const int16_t to[3],
                             int16_t hit[3], int16_t normal[3])
{
    BfmForestField *field = context;
    return field && from && to && hit &&
           bfm_forest_bridge_contact(&field->bridge, from, to, hit, normal);
}

int bfm_forest_bridge_shot(void *context, const int16_t from[3], const int16_t to[3],
                           int16_t hit[3])
{
    return bfm_forest_bridge_impact(context, from, to, hit, NULL);
}

int bfm_forest_tick_shore_actor(BfmForestField *field, const int16_t player[3])
{
    return bfm_forest_tick_shore_actor_with_run(field, player, NULL);
}

unsigned bfm_forest_take_player_strike(BfmForestField *field, int16_t *yaw)
{
    if (!field || !yaw || !field->player_hit_power) return 0;
    unsigned power = field->player_hit_power;
    *yaw = field->player_hit_yaw;
    field->player_hit_power = 0;
    return power;
}

int bfm_forest_take_player_hit(BfmForestField *field, int16_t *shot_yaw)
{
    return bfm_forest_take_player_strike(field, shot_yaw) != 0;
}

int bfm_forest_tick_shore_actor_with_run(BfmForestField *field, const int16_t player[3],
                                        BfmGameplayRun *run)
{
    return bfm_forest_tick_shore_actor_aim(field, player, player, run);
}

int bfm_forest_tick_shore_actor_aim(BfmForestField *field, const int16_t player[3],
                                   const int16_t guard_target[3], BfmGameplayRun *run)
{
    if (!field || !player || !guard_target) return 0;
    bfm_forest_drift_tick(&field->drift, run);
    unsigned drift_count = bfm_forest_drift_count(&field->drift);
    bfm_forest_fall_set_other_slots(&field->fall, drift_count, drift_count);
    bfm_forest_fall_tick_emitter(&field->fall, run, player);
    bfm_forest_glows_move(&field->lights, player[2], run);
    bfm_forest_boulders_tick(&field->boulders, player[2]);
    if (bfm_forest_shore_step(field->bank_patrol, field, player, guard_target, run) == BFM_SHORE_INVALID ||
        bfm_forest_shore_step(field->shore, field, player, guard_target, run) == BFM_SHORE_INVALID)
        return 0;
    bfm_forest_upper_trunks_tick(field->upper_trunks, player[2]);
    bfm_forest_tick_gateway(field, player);
    bfm_forest_bridge_approach(&field->bridge, player[2]);
    bfm_forest_shore_figures_tick(field->shore_figures, player);
    if (bfm_forest_trail_step(field->trail, player) == BFM_TRAIL_INVALID) return 0;
    if (run) {
        BfmForestTransform body = {0};
        for (unsigned axis = 0; axis < 3; ++axis) body.translation[axis] = player[axis];
        BfmForestShore *guards[FOREST_MELEE_GUARDS] = {field->bank_patrol, field->shore};
        for (unsigned i = 0; i < FOREST_MELEE_GUARDS && !field->player_hit_power; ++i) {
            int16_t yaw;
            const BfmForestTransform *strike = bfm_forest_shore_strike(guards[i], &yaw);
            if (strike && bfm_forest_soldier_hits_player(field, strike, &body)) {
                field->player_hit_power = 11;
                field->player_hit_yaw = yaw;
            }
        }
        if (!field->player_hit_power &&
            bfm_forest_shore_figures_bite(field->shore_figures, &field->player_hit_yaw))
            field->player_hit_power = 9;
        for (unsigned i = 0; i < FOREST_BANK_ROUNDS; ++i) {
            BfmForestRifleRound *round = &field->rounds[i];
            if (!round->active) continue;
            int blocked = bfm_forest_rifle_round_blocked(round, field);
            int contact = !blocked && bfm_forest_rifle_hits_player(field, &round->root, &body, 0);
            if (contact && !field->player_hit_power) {
                field->player_hit_power = 25;
                field->player_hit_yaw = round->yaw;
            }
            bfm_forest_rifle_round_tick(round, field->round_art, blocked || contact);
        }
        for (unsigned i = 0; i < BFM_FOREST_BANK_RIFLES; ++i) {
            BfmForestRifleShot shot;
            int result = bfm_forest_rifle_tick(field->rifles, i, player[2], run, &shot);
            if (!result) return 0;
            if (result == 2) {
                unsigned slot = 0;
                while (slot < FOREST_BANK_ROUNDS && field->rounds[slot].active) ++slot;
                if (slot == FOREST_BANK_ROUNDS ||
                    !bfm_forest_rifle_round_issue(&field->rounds[slot], field->round_art, &shot))
                    return 0;
            }
        }
    }
    unsigned scatter = bfm_forest_trail_scatter(field->trail);
    if (run && scatter) {
        for (unsigned i = 0; i < 4; ++i) {
            unsigned selector = bfm_gameplay_random(run);
            bfm_forest_fall_emit(&field->fall, run, player, selector, scatter);
        }
    }
    bfm_forest_glows_place(&field->lights, player[2], run);
    if (run && !bfm_forest_rifles_place(field->rifles, player[2])) return 0;
    bfm_forest_fall_step(&field->fall, run);
    return 1;
}

int bfm_forest_draw_entrance_player(BfmForestField *field, const BfmForestCamera *camera,
                           const BfmForestPose *pose, const BfmForestTransform *root,
                           uint32_t visible_parts, unsigned buffer, const BfmForestBladeFrame *frame,
                           uint32_t pixels[240][320])
{
    if (!field || !pose || !root) return 0;
    BfmForestActor actors[FOREST_FIELD_BODIES];
    size_t count = 0;
    for (unsigned i = 0; i < BFM_FOREST_UPPER_TRUNKS; ++i) {
        const BfmForestActor *trunk = bfm_forest_upper_trunk(field->upper_trunks, i);
        if (trunk) actors[count++] = *trunk;
    }
    for (unsigned role = 0; role < BFM_FOREST_BRIDGE_TARGET_COUNT; ++role) {
        if (field->bridge.logs[role].state != BFM_LOG_UNPLACED)
            actors[count++] = *bfm_forest_bridge_targets_actor(field->bridge.targets, role);
    }
    const BfmForestActor *lesson = bfm_forest_lesson_guard_actor(field->lesson_guard);
    if (lesson) actors[count++] = *lesson;
    const BfmForestActor *trunk = bfm_forest_trail_trunk(field->trail);
    if (trunk) actors[count++] = *trunk;
    count += bfm_forest_shore_figures_actors(field->shore_figures, actors + count,
                                            2 * BFM_FOREST_SHORE_FIGURES);
    const BfmForestActor *crossing = bfm_forest_trail_guard(field->trail);
    if (crossing) actors[count++] = *crossing;
    const BfmForestSubmittedFrame *submitted = &field->submitted[field->next_frame];
    if (submitted->fusion_actor.visible_parts) actors[count++] = submitted->fusion_actor;
    for (unsigned i = 0; i < FOREST_MELEE_GUARDS; ++i) {
        if (submitted->guard[i].parts)
            actors[count++] = (BfmForestActor){bfm_forest_shore_guard_mesh(field->shore),
                &submitted->guard[i].pose, &submitted->guard[i].root,
                submitted->guard[i].parts, &submitted->camera};
    }
    BfmForestDustCard cards[BFM_FOREST_EFFECT_CARDS + BFM_FOREST_WATER_EFFECT_CARDS + BFM_TETHER_CARDS +
                            BFM_FOREST_MUZZLE_CARDS + BFM_GUNSHOT_FAN_CARDS + BFM_FOREST_SHOT_EFFECT_CARDS];
    unsigned card_count = bfm_forest_dust_project(&field->dust, camera, cards);
    memcpy(cards + card_count, submitted->cards, submitted->count * sizeof *cards);
    card_count += submitted->count;
    const BfmForestAtmosphereFrame *atmosphere = &field->atmosphere[field->next_atmosphere];
    memcpy(actors + count, atmosphere->bodies, atmosphere->body_count * sizeof *actors);
    count += atmosphere->body_count;
    memcpy(cards + card_count, atmosphere->cards, atmosphere->count * sizeof *cards);
    card_count += atmosphere->count;
    card_count += bfm_forest_drift_project(&field->drift, cards + card_count);
    memcpy(cards + card_count, submitted->skill_cards, submitted->skill_count * sizeof *cards);
    card_count += (unsigned)submitted->skill_count;
    return bfm_forest_terrain_draw_entrance_with_effects(field->terrain, camera,
               field->player, pose, root, visible_parts, &field->blade, buffer, frame,
               actors, count, cards, card_count, pixels);
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

static int trunk_support(const BfmForestField *field, BfmForestBridgeProbe probe,
                         const int16_t from[3], const int16_t to[3], BfmForestContact *floor)
{
    int hit = bfm_forest_bridge_support(&field->bridge, probe, from, to, floor);
    BfmForestBridgeSurface upper;
    if (field->upper_trunks && bfm_forest_upper_trunks_surface(field->upper_trunks, probe,
            from, to, &upper) && (!hit || upper.height < floor->height)) {
        *floor = (BfmForestContact){.height = upper.height,
            .normal = {upper.normal[0], upper.normal[1], upper.normal[2]}, .bridge = 1};
        return 1;
    }
    return hit;
}

int bfm_forest_ground(const BfmForestField *field, int16_t x, int16_t y,
                      int16_t z, BfmForestContact *contact)
{
    if (!field || !contact)
        return 0;
    FloorPoint point = {(u16)x, (u16)y, (u16)z, 0};
    D_801D94F0 = (FloorGrid *)&field->grid;
    u32 slope = func_80134510(&point);
    D_801D94F0 = NULL;
    if (slope)
        *contact = (BfmForestContact){(int16_t)point.y,
            {(int16_t)D_801152A8.x, (int16_t)D_801152A8.y, (int16_t)D_801152A8.z},
            D_801D9500, (uint16_t)slope, 0};
    if (y >= INT16_MIN + 16) {
        int16_t near[3] = {x, (int16_t)(y - 16), z};
        int bottom = near[1] + 768;
        int16_t far[3] = {x, (int16_t)(bottom < INT16_MAX ? bottom : INT16_MAX), z};
        BfmForestContact log;
        if (trunk_support(field, BFM_BRIDGE_FLOOR_SEGMENT, near, far, &log) &&
            (!slope || log.height < contact->height)) {
            *contact = log;
            return 1;
        }
    }
    return slope != 0;
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

/* The north exit's unrotated box belongs to a distance-activated room actor.
 * The actor references the outgoing record. A bounded terrain selector in
 * its own cell requests that record when the player crosses the doorway. */
static void load_gateway(BfmForestField *field, const uint8_t *descriptor)
{
    if (read32(descriptor + 0xbc) != UINT32_C(0x8013e298))
        return;
    const uint8_t *table = bfm_forest_room_bytes(field, read32(descriptor + 0xc8), 8);
    const uint8_t *box = bfm_forest_room_bytes(field, UINT32_C(0x8018707c), 16);
    if (!table || !box || read32(box))
        return;
    uint32_t address = read32(table + 4);
    const uint8_t *record = bfm_forest_room_bytes(field, address, 36);
    if (!record || read16(record) != 0x3009)
        return;
    const uint8_t *row;
    uint32_t list = read32(descriptor + 0xb8);
    for (;;) {
        row = bfm_forest_room_bytes(field, list, 20);
        if (!row || !read16(row + 6))
            return;
        if (read16(row + 6) == 126 && read32(row + 16) == address)
            break;
        list += 20;
    }
    if (read16(row + 8) || read16(row + 10) || read16(row + 12))
        return;
    ForestGateway gate = {.record = record, .entry = read16(row + 14)};
    for (unsigned i = 0; i < 3; ++i) {
        gate.root[i] = (int16_t)read16(row + 2 * i);
        gate.low[i] = (int16_t)read16(box + 4 + 4 * i);
        gate.high[i] = (int16_t)read16(box + 6 + 4 * i);
        if (gate.low[i] > 0 || gate.high[i] < 0)
            return;
    }
    const FloorGrid *grid = &field->grid;
    ContactCell cell = contact_cell(grid, gate.root);
    for (unsigned i = 0; i < cell.count; ++i) {
        if (!(cell.tokens[i] & 0x8000))
            continue;
        const FloorFace *face = contact_face(grid, cell.tokens[i]);
        if (face->flags != 0x203 || signed_distance(grid, face->e[0], gate.root))
            continue;
        unsigned j = 1;
        while (j <= 4 && signed_distance(grid, face->e[j], gate.root) >= 0)
            ++j;
        if (j == 5) {
            if (gate.face)
                return;
            gate.face = face;
        }
    }
    field->gateway = gate;
}

void bfm_forest_tick_gateway(BfmForestField *field, const int16_t player[3])
{
    if (field && player && field->gateway.face && !field->gateway.active) {
        int gap = abs((int)player[2] - field->gateway.root[2]);
        field->gateway.active = gap >= 705 && gap < 768;
    }
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

/* A cell seam can pass through a single polygon. Test the bounded, unmarked
 * level surface under the foot using the faces indexed by the given cell. */
static int on_level_floor(const FloorGrid *grid, ContactCell cell, const int16_t p[3])
{
    for (unsigned i = 0; i < cell.count; ++i) {
        u16 token = cell.tokens[i];
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

/* Contact selector 3 checks a vertical segment from three units above the
 * feet to one unit below. Only the mounted exit owns this terrain face. */
static const FloorFace *gateway_crossing(const BfmForestField *field,
                                        const int16_t from[3], const int16_t to[3])
{
    const ForestGateway *gate = &field->gateway;
    if (!gate->active || abs(to[0] - from[0]) > 16 || abs(to[1] - from[1]) > 16 ||
        abs(to[2] - from[2]) > 16 ||
        crossing_margin(&field->grid, gate->face, 4, from, to) < -EDGE_TOLERANCE)
        return NULL;
    int x = to[0] - gate->root[0], y = to[1] - gate->root[1], z = to[2] - gate->root[2];
    if (x < gate->low[0] || x > gate->high[0] || z < gate->low[2] || z > gate->high[2] ||
        y + 1 < gate->low[1] || y - 3 > gate->high[1])
        return NULL;
    return gate->face;
}

int bfm_forest_cross_gateway(BfmForestField *field, const int16_t before[3],
                             const int16_t after[3], RoomTransferDescriptor *destination,
                             uint16_t *entry)
{
    if (!field || !before || !after || !destination || !entry || field->gateway.issued ||
        !gateway_crossing(field, before, after))
        return 0;
    bfm_forest_decode_transfer(field->gateway.record, destination);
    *entry = field->gateway.entry;
    field->gateway.issued = 1;
    return 1;
}

static const FloorPlane *shore_floor_crossing(const FloorGrid *grid, const int16_t near[3],
                                             const int16_t far[3], int cross_grade, unsigned *next_face)
{
    ContactCell cell = contact_cell(grid, near);
    for (unsigned i = *next_face; i < cell.count; ++i) {
        u16 token = cell.tokens[i];
        const FloorFace *face = contact_face(grid, token);
        const FloorPoint *n = &grid->verts[face->e[0].v];
        int tolerance = (s16)n->y < -3584 ? 0x1800 : EDGE_TOLERANCE;
        if ((s16)n->y >= 0 ||
            crossing_margin(grid, face, (token & 0x8000) ? 4 : 3, near, far) < -tolerance)
            continue;
        *next_face = i + 1;
        return face->flags || (n->x && !cross_grade) ? NULL : &face->e[0];
    }
    return NULL;
}

static BfmForestShoreFloor shore_floor_kind(const FloorGrid *grid, const FloorPlane *plane,
                                          int allow_reverse_grade)
{
    if (!plane) return BFM_SHORE_FLOOR_UNKNOWN;
    const FloorPoint *n = &grid->verts[plane->v];
    int nx = (s16)n->x, ny = (s16)n->y, nz = (s16)n->z;
    if (nx) {
        /* The dry trail also slopes across the direction of travel. */
        if (ny == -2 * abs(nx) && (!nz || abs(abs(nx) - 2 * abs(nz)) <= 1))
            return BFM_SHORE_FLOOR_GRADE;
        return BFM_SHORE_FLOOR_UNKNOWN;
    }
    if (ny < -3584) {
        if (!nz) return BFM_SHORE_FLOOR_FLAT;
        if (ny == 4 * nz || ny == 2 * nz || (allow_reverse_grade && ny == -2 * nz))
            return BFM_SHORE_FLOOR_GRADE;
    }
    if (ny >= -3019 && ny < -1400 && abs(nz) == -ny)
        return BFM_SHORE_FLOOR_STEEP;
    return BFM_SHORE_FLOOR_UNKNOWN;
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

typedef enum {
    WALL_SCAN_CELL, WALL_SCAN_BOUNDED, WALL_SCAN_TOWER_INTERIOR
} WallScan;

/* A correction can carry the endpoint into a second wall. Revisit the
 * mounted faces in their stored order after each correction. */
static int resolve_cell(const FloorGrid *grid, ContactCell cell, const int16_t from[3],
                        int16_t end[3], WallScan scan, const FloorFace *passage)
{
    if (!cell.count)
        return -1;
    for (unsigned i = 0; i < cell.count; ++i) {
        u16 token = cell.tokens[i];
        const FloorFace *face = contact_face(grid, token);
        if (face == passage)
            continue;
        const FloorPoint *n = &grid->verts[face->e[0].v];
        if ((s16)n->y < 0 && !(face->flags & 0x300))
            continue;
        unsigned sides = (token & 0x8000) ? 4 : 3;
        /* The tower interior's ordinary upright walls have no material flag. */
        int sliding = face->flags == 0x100 || face->flags == 0x2000 ||
                      (scan == WALL_SCAN_TOWER_INTERIOR && !face->flags);
        if (!sliding || n->y || (!n->x && !n->z)) {
            if (scan != WALL_SCAN_CELL && crossing_margin(grid, face, sides, from, end) < -EDGE_TOLERANCE)
                continue;
            return -1;
        }
        if (crossing_margin(grid, face, sides, from, end) >= -EDGE_TOLERANCE)
            return slide_wall(grid, face->e[0], end) ? 1 : -1;
    }
    return 0;
}

static int resolve_walls(const FloorGrid *grid, ContactCell start,
                         const int16_t from[3], int16_t end[3], WallScan scan,
                         const FloorFace *passage)
{
    for (unsigned pass = 0; pass < 6; ++pass) {
        int hit = resolve_cell(grid, start, from, end, scan, passage);
        if (hit == 0 && (!same_grid_index(from[0], end[0]) || !same_grid_index(from[2], end[2])))
            hit = resolve_cell(grid, contact_cell(grid, end), from, end, scan, passage);
        if (hit <= 0)
            return hit == 0;
    }
    return 0;
}

int bfm_room_walk_grid_level_step(const BfmRoomWalkGrid *walk, const int16_t from[3],
                                  int16_t to[3])
{
    if (!walk || !from || !to || from[1] != to[1] ||
        abs(to[0] - from[0]) > 16 || abs(to[2] - from[2]) > 16)
        return 0;
    const FloorGrid *grid = &walk->grid;
    ContactCell start = contact_cell(grid, from);
    int16_t foot[3] = {to[0], to[1], to[2]};
    if (!on_level_floor(grid, start, from) ||
        !resolve_walls(grid, start, from, foot, 1, NULL) ||
        !on_level_floor(grid, contact_cell(grid, foot), foot))
        return 0;
    memcpy(to, foot, sizeof foot);
    return 1;
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

/* Follow the centre of the doorway's grid column and bind the first
 * descending quarter-grade ahead. Identical normals occur much farther
 * into the forest, so retain the distance of this plane as well. */
static void load_entrance_descent(BfmForestField *field)
{
    const ForestEntrance *door = &field->entrances[0];
    if (door->yaw != 2048)
        return;
    int16_t foot[3] = {(s16)door->position[0], (s16)door->position[1], 0};
    int crossed_creek = 0;
    for (unsigned row = 0; row < field->grid.h; ++row) {
        int z = ((int)field->grid.z0 + (int)row) * 128 - 32768 + 64;
        if (z <= (s16)door->position[2] || z > INT16_MAX)
            continue;
        foot[2] = (s16)z;
        BfmForestContact ground;
        if (foot[1] < INT16_MIN + 128 ||
            !bfm_forest_ground(field, foot[0], (s16)(foot[1] - 128), foot[2], &ground))
            return;
        foot[1] = ground.height;
        if (!ground.flags && ground.slope == 0x2000 && !ground.normal[0] &&
            ground.normal[2] > 0 && ground.normal[1] == -4 * ground.normal[2]) {
            if (field->bank_start_z < field->bank_end_z)
                break;
            if (!field->first_descent)
                field->first_descent = support_plane(&field->grid, contact_cell(&field->grid, foot),
                                                     &ground, foot);
        } else if (field->first_descent) {
            /* The descent leads to a dry bank, one water cell and a second
             * flat at the same height. Stop at its next incline. */
            if (!crossed_creek && z - 64 == field->bank_end_z &&
                field->bank_start_z < field->bank_end_z &&
                ground.flags == 2 && ground.slope == 0x2000 &&
                !ground.normal[0] && !ground.normal[2] &&
                ground.height > field->bank_height) {
                crossed_creek = 1;
                continue;
            }
            if (!corridor_ground(&ground) || ground.normal[2])
                break;
            if (crossed_creek) {
                if (ground.height != field->bank_height)
                    break;
                ++field->far_bank_cells;
                continue;
            }
            field->bank_height = ground.height;
            if (field->bank_start_z == field->bank_end_z)
                field->bank_start_z = z - 64;
            field->bank_end_z = z + 64;
        }
    }
}

/* Unknown geometry shares the broad-phase cells of the flat bank. Only
 * permit an unattached face there when the actual swept segment misses it. */
static int creek_approach(const BfmForestField *field, const int16_t from[3],
                          const int16_t to[3])
{
    int x = (s16)field->entrances[0].position[0];
    if (field->bank_start_z >= field->bank_end_z || !same_grid_index(from[0], x) ||
        !same_grid_index(to[0], x))
        return 0;
    if (from[2] >= field->bank_start_z && from[2] <= field->bank_end_z &&
        to[2] >= field->bank_start_z && to[2] <= field->bank_end_z)
        return 1;
    int far_start = field->bank_end_z + 128;
    int far_end = far_start + 128 * (int)field->far_bank_cells;
    return field->far_bank_cells && from[2] >= far_start && to[2] >= far_start &&
           from[2] <= far_end && to[2] <= far_end;
}

static int descent_ground(const BfmForestField *field, const BfmForestContact *ground,
                          const int16_t at[3])
{
    if (!field->first_descent || ground->flags || ground->slope != 0x2000 ||
        !same_grid_index(at[0], (s16)field->entrances[0].position[0]) ||
        ground->height > INT16_MAX - 2)
        return 0;
    const FloorPoint *n = &field->grid.verts[field->first_descent->v];
    int16_t feet[3] = {at[0], (s16)(ground->height + 2), at[2]};
    return ground->normal[0] == (s16)n->x && ground->normal[1] == (s16)n->y &&
           ground->normal[2] == (s16)n->z &&
           llabs(signed_distance(&field->grid, *field->first_descent, feet)) <= -(s16)n->y;
}

/* Sample the terrain grid toward the lower integer coordinate on either side
 * of zero. The divisor must be positive. */
static int64_t grid_floor(int64_t numerator, int64_t denominator)
{
    int64_t quotient = numerator / denominator;
    return quotient - (numerator % denominator < 0);
}

static int floor_sweep_span(const int16_t from[3], const int16_t end[3])
{
    int dx = end[0] - from[0], dz = end[2] - from[2];
    int span = (int)sqrt((double)dx * dx + (double)dz * dz);
    if (span < 3)
        return 4;
    return span < 5 ? span + 1 : span;
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
    int span = floor_sweep_span(from, end);
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

/* Remove the whole-unit penetration, then move far enough along the
 * mounted face normal to clear the support. Each coordinate rounds its
 * displacement toward zero, including the small lateral correction. */
static int project_contact_plane(const FloorGrid *grid, const FloorPlane *plane, int depth,
                               int16_t end[3])
{
    const FloorPoint *n = &grid->verts[plane->v];
    int nx = (s16)n->x, ny = (s16)n->y, nz = (s16)n->z;
    int64_t bias = grid->dist[plane->d], far_y = end[1] + depth;
    int64_t distance = (int64_t)nx * end[0] + ny * far_y + (int64_t)nz * end[2] + bias;
    int64_t length_sq = (int64_t)nx * nx + (int64_t)ny * ny + (int64_t)nz * nz;
    int64_t projected_x = end[0] - distance * nx / length_sq;
    int64_t projected_y = far_y - distance * ny / length_sq;
    int64_t projected_z = end[2] - distance * nz / length_sq;
    for (int step = 3;; ++step) {
        int64_t x = projected_x + step * nx / 4096;
        int64_t y = projected_y + step * ny / 4096;
        int64_t z = projected_z + step * nz / 4096;
        if (x < INT16_MIN || x > INT16_MAX || y < INT16_MIN || y > INT16_MAX ||
            z < INT16_MIN || z > INT16_MAX)
            return 0;
        if (nx * x + ny * y + nz * z + bias >= (ny < -3584 ? 0x1800 : EDGE_TOLERANCE)) {
            end[0] = (int16_t)x;
            end[1] = (int16_t)y;
            end[2] = (int16_t)z;
            return 1;
        }
    }
}

int bfm_forest_bank_body_normal(const BfmForestField *field, const int16_t from[3],
                               const int16_t to[3], int16_t normal[3])
{
    if (!field || !from || !to || !normal || !field->far_bank_cells ||
        from[2] < field->bank_end_z + 128 || abs(to[0] - from[0]) > 16 ||
        abs(to[2] - from[2]) > 16 || from[1] < INT16_MIN + 4) return 0;
    /* The body probes eight units below its intended foot. The terrain
     * sweep also extends the sample according to horizontal travel. */
    int depth = 8 + floor_sweep_span(from, to) + 1;
    if (to[1] > INT16_MAX - depth) return 0;
    int16_t near[3] = {from[0], (int16_t)(from[1] - 4), from[2]};
    int16_t far[3] = {to[0], (int16_t)(to[1] + depth), to[2]};
    unsigned cursor = 0;
    const FloorPlane *plane = shore_floor_crossing(&field->grid, near, far, 1, &cursor);
    if (!plane) return 0;
    const FloorPoint *axis = &field->grid.verts[plane->v];
    normal[0] = (int16_t)axis->x;
    normal[1] = (int16_t)axis->y;
    normal[2] = (int16_t)axis->z;
    return 1;
}

enum ShoreWalker { SHORE_PATROL, SHORE_PURSUIT, SHORE_PLAYER };

static BfmForestShoreFloor shore_floor_step(const BfmForestField *field,
                                            const int16_t from[3], int16_t to[3],
                                            enum ShoreWalker walker)
{
    int max_step = walker == SHORE_PLAYER ? 16 : walker == SHORE_PURSUIT ? 8 : 5;
    if (!field || !from || !to || from[1] < INT16_MIN + 4 ||
        abs(to[0] - from[0]) > max_step || abs(to[2] - from[2]) > max_step)
        return BFM_SHORE_FLOOR_UNKNOWN;
    int depth = floor_sweep_span(from, to) + 1;
    if (to[1] > INT16_MAX - depth) return BFM_SHORE_FLOOR_UNKNOWN;

    const FloorGrid *grid = &field->grid;
    int16_t near[3] = {from[0], (int16_t)(from[1] - 4), from[2]};
    int16_t far[3] = {to[0], (int16_t)(to[1] + depth), to[2]};
    unsigned next_face = 0;
    const FloorPlane *first = shore_floor_crossing(grid, near, far, walker == SHORE_PLAYER, &next_face);
    BfmForestShoreFloor approach = shore_floor_kind(grid, first, walker != SHORE_PLAYER);
    /* A guard keeps its last foothold and turns when its stride meets
     * the steep bank, including when that face is the first contact. */
    if (walker != SHORE_PLAYER && approach == BFM_SHORE_FLOOR_STEEP)
        return approach;
    int16_t projected[3] = {to[0], to[1], to[2]};
    if ((approach != BFM_SHORE_FLOOR_FLAT && approach != BFM_SHORE_FLOOR_GRADE) ||
        !project_contact_plane(grid, first, depth, projected))
        return BFM_SHORE_FLOOR_UNKNOWN;
    if (walker == SHORE_PLAYER) {
        /* The primary sweep can leave a flat and enter a side grade in
         * the same grid cell. Each later face sees the adjusted endpoint. */
        const FloorPlane *next;
        while ((next = shore_floor_crossing(grid, near, projected, 1, &next_face))) {
            BfmForestShoreFloor surface = shore_floor_kind(grid, next, 0);
            if ((surface != BFM_SHORE_FLOOR_FLAT && surface != BFM_SHORE_FLOOR_GRADE) ||
                !project_contact_plane(grid, next, 0, projected))
                return BFM_SHORE_FLOOR_UNKNOWN;
        }
    }
    const FloorFace *passage = walker == SHORE_PLAYER ? gateway_crossing(field, from, projected) : NULL;
    if (!resolve_walls(grid, contact_cell(grid, near), near, projected, 1, passage) ||
        projected[1] > INT16_MAX - 6)
        return BFM_SHORE_FLOOR_UNKNOWN;

    int16_t below[3] = {projected[0], (int16_t)(projected[1] + 6), projected[2]};
    next_face = 0;
    const FloorPlane *second = shore_floor_crossing(grid, projected, below, walker == SHORE_PLAYER, &next_face);
    BfmForestShoreFloor footing = shore_floor_kind(grid, second, walker != SHORE_PLAYER);
    if (footing == BFM_SHORE_FLOOR_STEEP || footing == BFM_SHORE_FLOOR_UNKNOWN)
        return footing;

    int16_t first_x = projected[0], first_y = projected[1], first_z = projected[2];
    if (!project_contact_plane(grid, second, 6, projected)) return BFM_SHORE_FLOOR_UNKNOWN;
    to[0] = first_x;
    /* The runner keeps the primary height when the short probe below the
     * foot already touches another face. The patrol follows that probe. */
    to[1] = walker == SHORE_PLAYER ? first_y : projected[1];
    to[2] = first_z;
    return approach == BFM_SHORE_FLOOR_GRADE ? approach : footing;
}

BfmForestShoreFloor bfm_forest_shore_floor_step(const BfmForestField *field,
                                                const int16_t from[3], int16_t to[3])
{
    return shore_floor_step(field, from, to, SHORE_PATROL);
}

void bfm_forest_clip_field_walls(const BfmForestField *field, const int16_t from[3], int16_t to[3])
{
    int16_t foot[3] = {to[0], to[1], to[2]};
    if (resolve_walls(&field->grid, contact_cell(&field->grid, from), from, foot, WALL_SCAN_BOUNDED, NULL))
        memcpy(to, foot, sizeof foot);
}

BfmForestShoreFloor bfm_forest_pursuit_floor_step(const BfmForestField *field,
                                                  const int16_t from[3], int16_t to[3])
{
    return shore_floor_step(field, from, to, SHORE_PURSUIT);
}

/* Both sides of the raised obstacle carry the rim's material. Only their
 * measured, upward-facing inclines support this walking contact. */
static int raised_normal(unsigned flags, int nx, int ny, int nz)
{
    return flags == 0x2000 && ny == -3825 && (abs(nx) == 280 || abs(nx) == 283) &&
           abs(nz) == 1434 && (nx < 0) == (nz < 0);
}

/* Q16 feet move continuously across a slanted polygon edge. The contact
 * query still returns integer world coordinates after selecting its face. */
static double foot_distance(const FloorGrid *grid, FloorPlane plane, const int16_t p[3],
                            const uint32_t position[3])
{
    double distance = (double)signed_distance(grid, plane, p);
    if (position) {
        const FloorPoint *n = &grid->verts[plane.v];
        distance += ((s16)n->x * (double)(u16)position[0] +
                     (s16)n->y * (double)(u16)position[1] +
                     (s16)n->z * (double)(u16)position[2]) / 65536.0;
    }
    return distance;
}

static double foot_crossing_margin(const FloorGrid *grid, const FloorFace *face,
                                     unsigned sides, const int16_t near[3],
                                     const int16_t far[3], const uint32_t start_pos[3],
                                     const uint32_t end_pos[3])
{
    double start = foot_distance(grid, face->e[0], near, start_pos);
    double end = foot_distance(grid, face->e[0], far, end_pos);
    if (start < 0 || end >= 0)
        return -HUGE_VAL;
    double time = start / (start - end);
    double margin = HUGE_VAL;
    for (unsigned j = 1; j <= sides; ++j) {
        double first = foot_distance(grid, face->e[j], near, start_pos);
        double last = foot_distance(grid, face->e[j], far, end_pos);
        double distance = first + (last - first) * time;
        if (distance < margin)
            margin = distance;
    }
    return margin;
}

/* Prefer a polygon containing the crossing over its neighbor whose edge
 * tolerance overlaps the same point at the crest. */
static const FloorPlane *raised_motion_support(const FloorGrid *grid, ContactCell cell,
    const int16_t near[3], const int16_t far[3], const uint32_t start_pos[3],
    const uint32_t end_pos[3])
{
    const FloorPlane *edge = NULL;
    for (unsigned i = 0; i < cell.count; ++i) {
        u16 token = cell.tokens[i];
        const FloorFace *face = contact_face(grid, token);
        const FloorPoint *v = &grid->verts[face->e[0].v];
        if (!raised_normal(face->flags, (s16)v->x, (s16)v->y, (s16)v->z))
            continue;
        unsigned sides = (token & 0x8000) ? 4 : 3;
        double margin = foot_crossing_margin(grid, face, sides, near, far, start_pos, end_pos);
        if (margin >= 0)
            return &face->e[0];
        if (!edge && margin >= -EDGE_TOLERANCE)
            edge = &face->e[0];
    }
    return edge;
}

static inline const FloorPlane *raised_support(const FloorGrid *grid, ContactCell cell,
                                        const int16_t near[3], const int16_t far[3])
{
    return raised_motion_support(grid, cell, near, far, NULL, NULL);
}

/* Project whole-unit penetration and lateral clearance separately, with the
 * foot two units above the rounded contact height. */
static int project_whole_foot(const FloorGrid *grid, const FloorPlane *plane,
                              const int16_t far[3], int16_t foot[3])
{
    const FloorPoint *v = &grid->verts[plane->v];
    int nx = (s16)v->x, ny = (s16)v->y, nz = (s16)v->z;
    int64_t norm = nx * nx + ny * ny + nz * nz;
    int64_t depth = -signed_distance(grid, *plane, far);
    int64_t x = far[0] + nx * depth / norm + nx * 3 / 4096;
    int64_t y = far[1] + grid_floor(2 * ny * depth + norm, 2 * norm) - 2;
    int64_t z = far[2] + nz * depth / norm + nz * 3 / 4096;
    if (x < INT16_MIN || x > INT16_MAX || y < INT16_MIN || y > INT16_MAX ||
        z < INT16_MIN || z > INT16_MAX)
        return 0;
    foot[0] = (int16_t)x;
    foot[1] = (int16_t)y;
    foot[2] = (int16_t)z;
    return 1;
}

BfmForestTopStep bfm_forest_raised_motion(const BfmForestField *field,
    const BfmForestContact *previous, const int16_t from[3], int16_t to[3],
    const uint32_t start_pos[3], const uint32_t end_pos[3])
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
    if (!resolve_walls(grid, start, from, far, 0, NULL))
        return BFM_FOREST_TOP_BLOCKED;
    int span = (int)sqrt((double)(dx * dx + dz * dz));
    if (from[1] < INT16_MIN + 4 || far[1] > INT16_MAX - span - 1)
        return BFM_FOREST_TOP_BLOCKED;
    int16_t near[3] = {from[0], (int16_t)(from[1] - 4), from[2]};
    far[1] += span + 1;
    const FloorPlane *plane = raised_motion_support(grid, start, near, far, start_pos, end_pos);
    if (!plane)
        plane = raised_motion_support(grid, contact_cell(grid, to), near, far, start_pos, end_pos);
    if (!plane) {
        BfmForestContact below;
        if (start_pos && end_pos && far[0] == to[0] && far[2] == to[2] &&
            bfm_forest_ground(field, far[0], from[1], far[2], &below) &&
            corridor_ground(&below) && !below.normal[2] && below.height > from[1] + 4)
            return BFM_FOREST_TOP_FREE;
        return BFM_FOREST_TOP_BLOCKED;
    }

    int16_t foot[3];
    if (!project_whole_foot(grid, plane, far, foot))
        return BFM_FOREST_TOP_BLOCKED;
    BfmForestContact ground;
    if (!bfm_forest_ground(field, foot[0], foot[1], foot[2], &ground))
        return BFM_FOREST_TOP_BLOCKED;
    BfmForestTopStep result;
    if (raised_normal(ground.flags, ground.normal[0], ground.normal[1], ground.normal[2]) &&
        abs(ground.height - foot[1]) <= 4)
        result = BFM_FOREST_TOP_GROUNDED;
    else if (corridor_ground(&ground) && !ground.normal[2] && ground.height > foot[1] + 4)
        result = BFM_FOREST_TOP_LEFT;
    else
        return BFM_FOREST_TOP_BLOCKED;
    memcpy(to, foot, sizeof foot);
    return result;
}

BfmForestTopStep bfm_forest_raised_step(const BfmForestField *field,
    const BfmForestContact *previous, const int16_t from[3], int16_t to[3])
{
    return bfm_forest_raised_motion(field, previous, from, to, NULL, NULL);
}

/* The far foot can already cross the downward incline when the vertical
 * probe at the previous foot still sees the upper flat. At its toe the
 * last sloping update finishes above the lower flat. */
static int cross_descent(const BfmForestField *field, ContactCell cell,
                         const int16_t from[3], int16_t end[3])
{
    const FloorGrid *grid = &field->grid;
    const FloorPlane *plane = field->first_descent;
    if (!plane || !same_grid_index(from[0], (s16)field->entrances[0].position[0]) ||
        !same_grid_index(from[0], end[0]))
        return 0;
    int span = floor_sweep_span(from, end);
    if (from[1] < INT16_MIN + 4 || end[1] > INT16_MAX - span - 1)
        return 0;
    int16_t near[3] = {from[0], (s16)(from[1] - 4), from[2]};
    int16_t far[3] = {end[0], (s16)(end[1] + span + 1), end[2]};
    for (unsigned i = 0; i < cell.count; ++i) {
        u16 token = cell.tokens[i];
        const FloorFace *face = contact_face(grid, token);
        if (face->flags || face->e[0].v != plane->v || face->e[0].d != plane->d ||
            foot_crossing_margin(grid, face, (token & 0x8000) ? 4 : 3, near, far, NULL, NULL) < -EDGE_TOLERANCE)
            continue;
        const FloorPoint *n = &grid->verts[plane->v];
        int ny = (s16)n->y, nz = (s16)n->z;
        int64_t norm = ny * ny + nz * nz;
        int64_t z = end[2] - nz * signed_distance(grid, *plane, far) / norm;
        int64_t h = nz * z + grid->dist[plane->d];
        int64_t y = grid_floor(2 * h - ny, -2 * ny) - 2;
        if (y < INT16_MIN || y > INT16_MAX || z < INT16_MIN || z > INT16_MAX)
            return 0;
        end[1] = (s16)y;
        end[2] = (s16)z;
        return 1;
    }
    return 0;
}

static int entrance_floor_step(const BfmForestField *field, const BfmForestContact *previous,
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
    int on_descent = descent_ground(field, previous, from);
    int bank = creek_approach(field, from, clipped);
    if ((!corridor_ground(previous) && !on_descent) || abs(previous->height - from[1]) > 4 ||
        !resolve_walls(grid, start, from, clipped, bank, NULL))
        return 0;
    if (!cross_descent(field, start, from, clipped)) {
        /* The previous downward probe can retain the slope across its toe.
         * The next sweep has already left it and reaches the lower flat. */
        if (on_descent && creek_approach(field, clipped, clipped)) {
            if (!bfm_forest_ground(field, clipped[0], clipped[1], clipped[2], &ground) ||
                !corridor_ground(&ground) || ground.normal[2])
                return 0;
            clipped[1] = ground.height;
        } else if (!follow_corridor_floor(grid, start, previous, from, clipped))
            return 0;
    }
    if (!bfm_forest_ground(field, clipped[0], clipped[1], clipped[2], &ground) ||
        (!corridor_ground(&ground) && !descent_ground(field, &ground, clipped)) ||
        abs(ground.height - clipped[1]) > 4)
        return 0;
    to[0] = clipped[0];
    to[1] = clipped[1];
    to[2] = clipped[2];
    return 1;
}

/* The clearing's tagged ground carries the same level surface as the
 * untagged approach. The creek bed and raised logs use other materials. */
static int dry_trail_surface(const BfmForestContact *ground)
{
    return ground->slope == 0x2000 && (!ground->flags ||
        (ground->flags == 0x1e && !ground->normal[0] && !ground->normal[2]));
}

int bfm_forest_jump_surface(const BfmForestContact *ground)
{
    return ground && (ground->bridge || dry_trail_surface(ground) ||
                      (ground->slope == 0x2000 && ground->flags == 0x2000));
}

/* The flat doorway landing has a material tag distinct from the helical
 * treads. Both carry the player's feet; the door actor handles entry. */
static int tower_walkable(const BfmForestContact *floor)
{
    return floor->slope == 0x2000 && (floor->flags == 0 || floor->flags == 3);
}

static BfmForestTopStep tower_foot_sweep(const BfmForestField *field,
    const BfmForestContact *previous, const int16_t from[3], int16_t to[3])
{
    if (!previous || !from || !to ||
        (!tower_walkable(previous) && !(field->room == SUMMIT_ROOM && previous->flags == 0x4000)) ||
        abs(previous->height - from[1]) > 4 ||
        from[1] != to[1] || abs(to[0] - from[0]) > 16 || abs(to[2] - from[2]) > 16)
        return BFM_FOREST_TOP_BLOCKED;
    int depth = floor_sweep_span(from, to) + 1;
    if (from[1] < INT16_MIN + 4 || to[1] > INT16_MAX - depth) return BFM_FOREST_TOP_BLOCKED;
    int16_t near[3] = {from[0], (int16_t)(from[1] - 4), from[2]};
    int16_t far[3] = {to[0], (int16_t)(to[1] + depth), to[2]};
    const FloorGrid *grid = &field->grid;
    ContactCell cell = contact_cell(grid, near);
    int grounded = 0;
    /* Ramps meet vertical faces at their seams. Resolve faces in the mounted
     * order and retest the corrected segment before deciding it left a tread. */
    for (unsigned pass = 0; pass < 6; ++pass) {
        unsigned i;
        for (i = 0; i < cell.count; ++i) {
            u16 token = cell.tokens[i];
            const FloorFace *face = contact_face(grid, token);
            const FloorPoint *n = &grid->verts[face->e[0].v];
            int tolerance = (s16)n->y < -3584 ? 0x1800 : EDGE_TOLERANCE;
            if ((face->flags & 0x200) ||
                crossing_margin(grid, face, (token & 0x8000) ? 4 : 3, near, far) < -tolerance)
                continue;
            if (!project_contact_plane(grid, &face->e[0], 0, far)) return BFM_FOREST_TOP_BLOCKED;
            if ((s16)n->y < -3019) grounded = 1;
            break;
        }
        if (i == cell.count) {
            if (!grounded) return BFM_FOREST_TOP_FREE;
            memcpy(to, far, sizeof far);
            return BFM_FOREST_TOP_GROUNDED;
        }
    }
    return BFM_FOREST_TOP_BLOCKED;
}

int bfm_forest_escape_floor_step(const BfmForestField *field, const int16_t from[3], int16_t to[3])
{
    BfmForestContact previous;
    return field && field->room == SUMMIT_ROOM &&
           bfm_forest_ground(field, from[0], from[1] - 16, from[2], &previous) &&
           tower_foot_sweep(field, &previous, from, to) == BFM_FOREST_TOP_GROUNDED;
}

/* The broad standing probe and the walking sweep use different edge
 * tolerances. Both must see a tread under a resolved foot. */
static int tower_supports_foot(const BfmForestField *field, const int16_t foot[3])
{
    BfmForestContact support;
    if (!bfm_forest_ground(field, foot[0], foot[1], foot[2], &support) ||
        !tower_walkable(&support) || abs(support.height - foot[1]) > 4)
        return 0;
    const FloorGrid *grid = &field->grid;
    ContactCell cell = contact_cell(grid, foot);
    int y = foot[1];
    if (y < INT16_MIN + 4 || y > INT16_MAX - 7)
        return 0;
    int16_t near[3] = {foot[0], (int16_t)(y - 4), foot[2]};
    int16_t far[3] = {foot[0], (int16_t)(y + 7), foot[2]};
    for (unsigned i = 0; i < cell.count; ++i) {
        u16 token = cell.tokens[i];
        const FloorFace *face = contact_face(grid, token);
        const FloorPoint *n = &grid->verts[face->e[0].v];
        if (face->flags != support.flags || (s16)n->y >= -3019)
            continue;
        int tolerance = (s16)n->y < -3584 ? 0x1800 : EDGE_TOLERANCE;
        if (crossing_margin(grid, face, (token & 0x8000) ? 4 : 3, near, far) >= -tolerance)
            return 1;
    }
    return 0;
}


BfmForestTopStep bfm_tower_ascent_floor_step(const BfmForestField *field,
    const BfmForestContact *previous, const int16_t from[3], int16_t to[3])
{
    if (!field || !field->tower_floor_raised) return BFM_FOREST_TOP_BLOCKED;
    if (!to) return BFM_FOREST_TOP_BLOCKED;
    int16_t foot[3];
    memcpy(foot, to, sizeof foot);
    BfmForestTopStep result = tower_foot_sweep(field, previous, from, foot);
    if (result == BFM_FOREST_TOP_GROUNDED) {
        if (!tower_supports_foot(field, foot)) return BFM_FOREST_TOP_BLOCKED;
        memcpy(to, foot, sizeof foot);
    }
    return result;
}

/* On the upper dry trail, one step can cross several triangles with
 * different normals. The room grid picks the bounded surface under the
 * proposed foot. The step must still clear walls and stay within the
 * vertical reach of grounded motion; a ledge requires an airborne action. */
int bfm_forest_corridor_step(const BfmForestField *field, const BfmForestContact *previous,
                             const int16_t from[3], int16_t to[3])
{
    if (!field || !previous || !from || !to)
        return 0;
    if (field->tower_floor_raised)
        return bfm_tower_ascent_floor_step(field, previous, from, to) == BFM_FOREST_TOP_GROUNDED;
    if (field->room == TOWER_INTERIOR_ROOM) {
        /* Inside the tower, successive turns overlap in X/Z. The foot sweeps
         * the nearby tread and projects all three coordinates along its normal.
         * A vertical floor lookup alone misses lateral travel on the incline.
         * The level summit landing continues onto its marked exit selector. */
        BfmForestContact ahead;
        if (previous->normal[0] || previous->normal[2] || previous->flags == 3 ||
            (bfm_forest_ground(field, to[0], from[1], to[2], &ahead) &&
             (ahead.normal[0] || ahead.normal[2] || ahead.flags == 3) &&
             abs(ahead.height - from[1]) <= 4))
            return tower_foot_sweep(field, previous, from, to) == BFM_FOREST_TOP_GROUNDED;
    }
    /* The far bank's ordinary flats and grades use successive terrain
     * sweeps for advancing actors. The second probe can find the next
     * incline while the first is still on the flat. */
    if (field->far_bank_cells && from[2] >= field->bank_end_z + 128 &&
        from[1] == to[1] && !previous->flags && abs(previous->height - from[1]) <= 4) {
        BfmForestShoreFloor support = shore_floor_step(field, from, to, SHORE_PLAYER);
        if (support == BFM_SHORE_FLOOR_FLAT || support == BFM_SHORE_FLOOR_GRADE)
            return 1;
    }
    if (entrance_floor_step(field, previous, from, to))
        return 1;
    if (from[1] != to[1] || !dry_trail_surface(previous) ||
        abs(previous->height - from[1]) > 4 ||
        abs(to[0] - from[0]) > 16 || abs(to[2] - from[2]) > 16)
        return 0;
    const FloorGrid *grid = &field->grid;
    int16_t foot[3] = {to[0], to[1], to[2]};
    WallScan scan = field->room == TOWER_INTERIOR_ROOM ? WALL_SCAN_TOWER_INTERIOR : WALL_SCAN_BOUNDED;
    if (!resolve_walls(grid, contact_cell(grid, from), from, foot, scan,
                       gateway_crossing(field, from, foot)))
        return 0;
    int reach = floor_sweep_span(from, foot) + 1;
    if (from[1] < INT16_MIN + reach)
        return 0;
    BfmForestContact support;
    if (!bfm_forest_ground(field, foot[0], (int16_t)(from[1] - reach), foot[2], &support) ||
        !dry_trail_surface(&support) || abs(support.height - from[1]) > reach)
        return 0;
    to[0] = foot[0];
    to[1] = support.height;
    to[2] = foot[2];
    return 1;
}

BfmForestTopStep bfm_forest_step_on_bridge(const BfmForestField *field,
                                           const int16_t from[3], int16_t to[3])
{
    int16_t foot[3] = {to[0], to[1], to[2]};
    const FloorGrid *grid = &field->grid;
    if (!resolve_walls(grid, contact_cell(grid, from), from, foot, 1, NULL))
        return BFM_FOREST_TOP_BLOCKED;
    BfmForestContact contact;
    if (!trunk_support(field, BFM_BRIDGE_FLOOR_STRIDE, from, foot, &contact)) {
        if (!bfm_forest_ground(field, foot[0], from[1], foot[2], &contact))
            return BFM_FOREST_TOP_BLOCKED;
        if (contact.height > from[1] + 4) return BFM_FOREST_TOP_FREE;
        if (!dry_trail_surface(&contact) || abs(contact.height - from[1]) > 4)
            return BFM_FOREST_TOP_BLOCKED;
    }
    to[0] = foot[0];
    to[1] = contact.height;
    to[2] = foot[2];
    return BFM_FOREST_TOP_GROUNDED;
}

int bfm_forest_doorway_step(const BfmForestField *field, const int16_t origin[3],
                            const int16_t from[3], int16_t to[3])
{
    if (!field || !origin || !from || !to || origin[1] != from[1])
        return 0;
    ContactCell doorway = contact_cell(&field->grid, origin);
    BfmForestContact ground;
    int16_t end[3] = {to[0], to[1], to[2]};
    if (!on_level_floor(&field->grid, doorway, from) ||
        !bfm_forest_ground(field, from[0], from[1], from[2], &ground) ||
        !bfm_forest_corridor_step(field, &ground, from, end) ||
        end[1] != from[1] || !on_level_floor(&field->grid, doorway, end))
        return 0;
    to[0] = end[0];
    to[2] = end[2];
    return 1;
}

static int creek_cell(const BfmForestField *field, const int16_t p[3])
{
    int x = (s16)field->entrances[0].position[0];
    return field->far_bank_cells && same_grid_index(p[0], x) &&
           p[2] >= field->bank_end_z && p[2] < field->bank_end_z + 128;
}

/* Only the marked horizontal faces in the first creek receive the player
 * when a downward foot sweep crosses the surface. The banks, buried bed
 * and empty cells do not trigger water entry. */
static int first_creek_water(const BfmForestField *field, const int16_t end[3],
                            const BfmForestContact *ground)
{
    return field->room == FOREST_ROOM && field->far_bank_cells &&
           end[2] >= field->bank_end_z && end[2] < field->bank_end_z + 128 &&
           ground->flags == 2 && ground->slope == 0x2000 && !ground->normal[0] &&
           ground->normal[1] < 0 && !ground->normal[2] && ground->height > field->bank_height;
}

int bfm_forest_dry_creek_bank(const BfmForestField *field, const int16_t foot[3])
{
    if (!field || !foot || field->room != FOREST_ROOM || !field->far_bank_cells ||
        foot[1] != field->bank_height || foot[2] < field->bank_start_z ||
        foot[2] > field->bank_end_z + 128 * (1 + (int)field->far_bank_cells))
        return 0;
    int16_t channel[3] = {foot[0], foot[1], (int16_t)(field->bank_end_z + 64)};
    BfmForestContact ground, water;
    return bfm_forest_ground(field, foot[0], foot[1], foot[2], &ground) &&
           !ground.bridge && !ground.flags && ground.slope == 0x2000 &&
           !ground.normal[0] && ground.normal[1] < 0 && !ground.normal[2] && ground.height == foot[1] &&
           bfm_forest_ground(field, channel[0], channel[1], channel[2], &water) &&
           first_creek_water(field, channel, &water);
}

/* A level stride over the near edge spends its final ground update above
 * the steep lip. The next player update begins an ordinary unsupported
 * descent. Bind the starting dry foot, the actual downhill facet and
 * the marked channel in the same column before releasing that support. */
static int starts_creek_bank_stride(const BfmForestField *field, const int16_t from[3],
                                    const int16_t end[3])
{
    return field && from && end && end[1] == from[1] &&
           end[2] > from[2] && end[2] - from[2] <= 16 && abs(end[0] - from[0]) <= 16 &&
           creek_cell(field, end) && same_grid_index(from[0], end[0]) &&
           bfm_forest_dry_creek_bank(field, from);
}

int bfm_forest_walks_off_creek_bank(const BfmForestField *field, const int16_t from[3],
                                    const int16_t end[3])
{
    if (!starts_creek_bank_stride(field, from, end)) return 0;
    BfmForestContact lip;
    if (!bfm_forest_ground(field, end[0], from[1], end[2], &lip) || lip.bridge || lip.flags ||
        lip.slope != 0x4000 || lip.normal[0] || lip.normal[1] >= 0 ||
        lip.normal[2] != -lip.normal[1] || lip.height <= from[1])
        return 0;
    const FloorGrid *grid = &field->grid;
    if (!support_plane(grid, contact_cell(grid, end), &lip, end)) return 0;
    int16_t foot[3] = {end[0], end[1], end[2]};
    return resolve_walls(grid, contact_cell(grid, from), from, foot, WALL_SCAN_BOUNDED, NULL) &&
           !memcmp(foot, end, sizeof foot);
}

/* Only the level-X, downhill 45-degree face between the first dry bank
 * and its marked channel supports this slide. Neighboring diagonal banks
 * and the far uphill lip have different normals. */
static const FloorPlane *creek_grade_at(const BfmForestField *field, const int16_t p[3])
{
    BfmForestContact g;
    if (!creek_cell(field, p) || p[2] >= field->bank_end_z + 64 ||
        !bfm_forest_ground(field, p[0], field->bank_height, p[2], &g) ||
        g.bridge || g.flags || g.normal[0] || g.normal[1] >= 0)
        return NULL;
    const FloorGrid *grid = &field->grid;
    ContactCell cell = contact_cell(grid, p);
    if (g.slope == 0x4000 && g.normal[2] == -g.normal[1])
        return support_plane(grid, cell, &g, p);

    /* Immediately past the dry edge, the ground reader's polygon tolerance
     * can still select the flat. Require a physical point inside the
     * adjoining incline, with no edge tolerance, before moving onto it. */
    if (g.slope != 0x2000 || g.normal[2] || g.height != field->bank_height ||
        p[2] <= field->bank_end_z) return NULL;
    for (unsigned i = 0; i < cell.count; ++i) {
        u16 token = cell.tokens[i];
        const FloorFace *face = contact_face(grid, token);
        FloorPlane plane = face->e[0];
        const FloorPoint *n = &grid->verts[plane.v];
        int ny = (s16)n->y, nz = (s16)n->z;
        if (face->flags || n->x || ny >= 0 || nz != -ny) continue;
        int64_t bias = grid->dist[plane.d];
        int64_t rim_y = grid_floor((int64_t)nz * field->bank_end_z + bias, -ny);
        if (rim_y != field->bank_height + 2) continue;
        int64_t y = grid_floor((int64_t)nz * p[2] + bias, -ny);
        if (y < INT16_MIN || y > INT16_MAX) continue;
        int16_t hit[3] = {p[0], (int16_t)y, p[2]};
        unsigned sides = (token & 0x8000) ? 4 : 3;
        unsigned j = 1;
        while (j <= sides && signed_distance(grid, face->e[j], hit) >= 0) ++j;
        if (j > sides) return &face->e[0];
    }
    return NULL;
}

static int creek_stride_clear(const BfmForestField *field, const int16_t from[3],
                              const int16_t end[3])
{
    if (!field || !from || !end || field->room != FOREST_ROOM || !creek_cell(field, end) ||
        !same_grid_index(from[0], end[0]) ||
        abs(end[0] - from[0]) > 16 || abs(end[2] - from[2]) > 16 ||
        abs(end[1] - from[1]) > 32)
        return 0;
    int16_t checked[3] = {end[0], end[1], end[2]};
    return resolve_walls(&field->grid, contact_cell(&field->grid, from), from, checked,
                         WALL_SCAN_BOUNDED, NULL) && !memcmp(checked, end, sizeof checked);
}

/* The grid contact is two units above the physical face. The sliding foot
 * on this grade is two further vertical units clear of that contact. */
static int creek_grade_foot_y(const FloorGrid *grid, const FloorPlane *plane, int z)
{
    const FloorPoint *n = &grid->verts[plane->v];
    return (int)grid_floor((int64_t)(s16)n->z * z + grid->dist[plane->d], -(s16)n->y) - 4;
}

static int above_creek_water(const BfmForestField *field, const int16_t p[3])
{
    BfmForestContact g;
    return bfm_forest_ground(field, p[0], field->bank_height, p[2], &g) &&
           first_creek_water(field, p, &g) && p[1] < g.height;
}

static int project_creek_stride(const BfmForestField *field, const FloorPlane *plane,
                                const int16_t from[3], int depth, int16_t end[3])
{
    int16_t far[3] = {end[0], end[1], end[2]}, foot[3];
    if (far[1] > INT16_MAX - depth) return 0;
    far[1] += (int16_t)depth;
    if (!project_whole_foot(&field->grid, plane, far, foot)) return 0;
    int y = creek_grade_foot_y(&field->grid, plane, foot[2]);
    if (y < INT16_MIN || y > INT16_MAX) return 0;
    foot[1] = (int16_t)y;
    if (!creek_stride_clear(field, from, foot)) return 0;
    const FloorPlane *next = creek_grade_at(field, foot);
    if (next != plane && (next || foot[2] <= from[2] || !above_creek_water(field, foot)))
        return 0;
    memcpy(end, foot, sizeof foot);
    return 1;
}

int bfm_forest_creek_bank_stride(const BfmForestField *field, const int16_t from[3], int16_t end[3])
{
    if (!starts_creek_bank_stride(field, from, end)) return 0;
    const FloorPlane *plane = creek_grade_at(field, end);
    if (!plane) return 0;
    int drop = creek_grade_foot_y(&field->grid, plane, end[2]) + 2 - from[1];
    if (drop > 4) return bfm_forest_walks_off_creek_bank(field, from, end);
    if (drop < 1) return 0;
    return project_creek_stride(field, plane, from, floor_sweep_span(from, end) + 1, end) ? 2 : 0;
}

/* The failed air sweep can end exactly on the physical face. Test the
 * bounded intersection at equality as well as a sweep through the plane. */
static int hits_creek_face(const FloorGrid *grid, const FloorFace *face, unsigned sides,
                           const int16_t from[3], const int16_t end[3])
{
    if (signed_distance(grid, face->e[0], end) != 0)
        return crossing_margin(grid, face, sides, from, end) >= -EDGE_TOLERANCE;
    if (signed_distance(grid, face->e[0], from) <= 0) return 0;
    for (unsigned i = 1; i <= sides; ++i)
        if (signed_distance(grid, face->e[i], end) < -EDGE_TOLERANCE) return 0;
    return 1;
}

int bfm_forest_creek_slope_contact(const BfmForestField *field, const int16_t from[3],
                                   int16_t end[3])
{
    if (!creek_stride_clear(field, from, end) || end[1] <= from[1] || end[1] > INT16_MAX - 2 ||
        !same_grid_index(from[2], end[2])) return 0;
    const FloorGrid *grid = &field->grid;
    const FloorPlane *plane = creek_grade_at(field, end);
    if (!plane) return 0;
    ContactCell cell = contact_cell(grid, end);
    /* Use the same two-unit foot sweep that stopped the ordinary air step.
     * Compare all competing faces on that same segment. */
    int16_t near[3] = {from[0], (int16_t)(from[1] + 2), from[2]};
    int16_t far[3] = {end[0], (int16_t)(end[1] + 2), end[2]};
    int64_t start = signed_distance(grid, *plane, near);
    int64_t travel = start - signed_distance(grid, *plane, far);
    int reached = 0;
    for (unsigned i = 0; i < cell.count; ++i) {
        u16 token = cell.tokens[i];
        const FloorFace *face = contact_face(grid, token);
        if ((s16)grid->verts[face->e[0].v].y >= 0 ||
            !hits_creek_face(grid, face, (token & 0x8000) ? 4 : 3, near, far)) continue;
        if (&face->e[0] == plane) {
            reached = 1;
        } else {
            int64_t a = signed_distance(grid, face->e[0], near);
            int64_t b = a - signed_distance(grid, face->e[0], far);
            if (a * travel <= start * b) return 0;
        }
    }
    return reached && project_creek_stride(field, plane, from, 0, end);
}

int bfm_forest_creek_slope_step(const BfmForestField *field, const int16_t from[3],
                                int16_t end[3], int16_t *normal_z)
{
    if (!creek_stride_clear(field, from, end) || end[2] < from[2] ||
        !same_grid_index(from[2], end[2])) return 0;
    const FloorGrid *grid = &field->grid;
    const FloorPlane *plane = creek_grade_at(field, from);
    if (plane) {
        if (abs(from[1] - creek_grade_foot_y(grid, plane, from[2])) > 1) return 0;
        if (!project_creek_stride(field, plane, from, 0, end)) return 0;
        if (normal_z) *normal_z = (s16)grid->verts[plane->v].z;
        return 1;
    }
    /* One supported update can carry the foot past the bounded toe. On
     * the following update it is above the adjacent water; the caller
     * integrates free fall from the current foot with its airborne speed. */
    if (!above_creek_water(field, from) || from[2] < INT16_MIN + 16) return 0;
    int16_t behind[3] = {from[0], from[1], (int16_t)(from[2] - 16)};
    plane = creek_grade_at(field, behind);
    if (!plane || abs(from[1] - creek_grade_foot_y(grid, plane, from[2])) > 1) return 0;
    if (normal_z) *normal_z = (s16)grid->verts[plane->v].z;
    return 2;
}

/* Classify only a descending sweep onto the selected bounded water face.
 * Its floor probe reports two units above the actual plane. A foot inside
 * that gap can finish the approach in the air, then enter on its next step. */
static int creek_water_descent(const BfmForestField *field, const int16_t from[3],
                               const int16_t end[3], const BfmForestContact *ground, int *entered)
{
    int plane_y = ground->height + 2;
    if (end[1] <= from[1] || from[1] > plane_y || end[1] < ground->height ||
        !same_grid_index(from[0], end[0]) || !same_grid_index(from[2], end[2]))
        return 0;
    BfmForestContact previous;
    if (!bfm_forest_ground(field, from[0], field->bank_height, from[2], &previous) ||
        (from[1] >= previous.height && !first_creek_water(field, from, &previous)))
        return 0;
    const FloorGrid *grid = &field->grid;
    ContactCell cell = contact_cell(grid, end);
    const FloorPlane *water = support_plane(grid, cell, ground, end);
    if (!water) return 0;
    int gap = end[1] <= plane_y;
    int16_t probe[3] = {end[0], gap ? (int16_t)(plane_y + 1) : end[1], end[2]};
    int64_t water_start = signed_distance(grid, *water, from);
    int64_t water_travel = water_start - signed_distance(grid, *water, probe);
    int16_t near[3] = {from[0], (int16_t)(from[1] + 2), from[2]};
    int16_t far[3] = {end[0], (int16_t)(end[1] + 2), end[2]};
    int reached = 0;
    for (unsigned i = 0; i < cell.count; ++i) {
        u16 token = cell.tokens[i];
        const FloorFace *face = contact_face(grid, token);
        unsigned sides = (token & 0x8000) ? 4 : 3;
        const FloorPlane plane = face->e[0];
        const FloorPoint *n = &grid->verts[plane.v];
        if ((s16)n->y >= 0) continue;
        /* Adjacent marked tiles may share this water plane at a cell
         * edge. Compare the plane equation and material, not its address. */
        int same_water = face->flags == ground->flags &&
                         !n->x && (s16)n->y == ground->normal[1] && !n->z &&
                         grid->dist[plane.d] == grid->dist[water->d];
        if (gap && !same_water &&
            crossing_margin(grid, face, sides, near, far) >= -EDGE_TOLERANCE)
            return 0;
        if (crossing_margin(grid, face, sides, from, probe) < -EDGE_TOLERANCE) continue;
        if (same_water) {
            reached = 1;
        } else {
            int64_t start = signed_distance(grid, face->e[0], from);
            int64_t travel = start - signed_distance(grid, face->e[0], probe);
            if (start * water_travel <= water_start * travel) return 0;
        }
    }
    *entered = reached && !gap;
    return reached;
}

/* The steep sides and water may be below a jump, but cannot land the player.
 * At lower altitudes, sweep the feet against the bounded floor faces too. */
static int clears_creek(const BfmForestField *field, const int16_t from[3],
                        const int16_t end[3], const BfmForestContact *ground)
{
    if (!same_grid_index(from[0], end[0]) || ground->height < field->bank_height ||
        ground->normal[0] || ground->normal[1] >= 0 ||
        !((ground->flags == 2 && ground->slope == 0x2000 && !ground->normal[2]) ||
          (!ground->flags && ground->slope == 0x4000 &&
           abs(ground->normal[2]) == -ground->normal[1])))
        return 0;
    if (from[1] < field->bank_height && end[1] < field->bank_height)
        return 1;
    BfmForestContact previous;
    if (end[1] >= ground->height ||
        !bfm_forest_ground(field, from[0], field->bank_height, from[2], &previous) ||
        from[1] >= previous.height)
        return 0;
    const FloorGrid *grid = &field->grid;
    ContactCell cell = contact_cell(grid, end);
    /* The ground reader keeps the feet two units above the plane. */
    int16_t near[3] = {from[0], (s16)(from[1] + 2), from[2]};
    int16_t far[3] = {end[0], (s16)(end[1] + 2), end[2]};
    for (unsigned i = 0; i < cell.count; ++i) {
        u16 token = cell.tokens[i];
        const FloorFace *face = contact_face(grid, token);
        if ((s16)grid->verts[face->e[0].v].y < 0 &&
            crossing_margin(grid, face, (token & 0x8000) ? 4 : 3, near, far) >= -EDGE_TOLERANCE)
            return 0;
    }
    return 1;
}

/* Sweep the actor base while it is above the entrance floor. The ordinary
 * downward probe starts above the proposed contact so a thin raised surface
 * is not lost after the feet pass through it. */
int bfm_forest_player_air_step(const BfmForestField *field, const int16_t from[3],
                               int16_t to[3], BfmForestContact *contact, int *landed, int *water)
{
    if (water) *water = 0;
    if (!field || !from || !to || !contact || !landed ||
        abs(to[0] - from[0]) > 16 || abs(to[2] - from[2]) > 16 ||
        abs(to[1] - from[1]) > 32)
        return 0;
    const FloorGrid *grid = &field->grid;
    int16_t end[3] = {to[0], to[1], to[2]};
    if (!resolve_walls(grid, contact_cell(grid, from), from, end, 1, NULL))
        return 0;
    BfmForestContact ground;
    int16_t upper = end[1] < from[1] ? end[1] : from[1];
    int creek = creek_cell(field, end);
    /* A probe starting underwater could see the unmarked bed instead. */
    if (creek && upper > field->bank_height)
        upper = (s16)field->bank_height;
    if (!bfm_forest_ground(field, end[0], upper, end[2], &ground))
        return 0;
    int marked = water && first_creek_water(field, end, &ground);
    if (!bfm_forest_jump_surface(&ground) &&
        !((creek || marked) && clears_creek(field, from, end, &ground))) {
        if (!marked || !creek_water_descent(field, from, end, &ground, water)) return 0;
        if (*water) {
            end[1] = ground.height;
            memcpy(to, end, sizeof end);
            *contact = ground;
            *landed = 1;
            return 1;
        }
    }
    int hit = end[1] >= from[1] && end[1] > ground.height + 2;
    /* A wall slide may move the downward probe just inside a raised face's
     * edge tolerance. The feet can land there only if their own sweep
     * crossed a bounded face before the wall displaced the endpoint. */
    if (hit && (to[0] != end[0] || to[2] != end[2]) &&
        raised_normal(ground.flags, ground.normal[0], ground.normal[1], ground.normal[2]) &&
        !raised_support(grid, contact_cell(grid, from), from, to) &&
        !raised_support(grid, contact_cell(grid, to), from, to)) {
        /* The reported floor has two units of clearance and the probe starts
         * four units above its input. Begin one unit below the rejected face. */
        int below = ground.height + 2 + 4 + 1;
        if (below > INT16_MAX ||
            !bfm_forest_ground(field, end[0], (s16)below, end[2], &ground) ||
            !corridor_ground(&ground))
            return 0;
        hit = end[1] > ground.height + 2;
    }
    if (hit) {
        int64_t nx = ground.normal[0], ny = ground.normal[1], nz = ground.normal[2];
        if ((nx || nz) && !ground.bridge) {
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

int bfm_forest_air_step(const BfmForestField *field, const int16_t from[3],
                        int16_t to[3], BfmForestContact *contact, int *landed)
{
    return bfm_forest_player_air_step(field, from, to, contact, landed, NULL);
}

/* Resolve the earliest indexed plain floor that the body segment
 * enters. Keep the separate integer rounding of normal projection and
 * outward clearance, including on tilted creek banks. */
static int thrown_ground_cell(const FloorGrid *grid, ContactCell cell,
                              const int16_t from[3], int16_t end[3], int road)
{
    for (unsigned i = 0; i < cell.count; ++i) {
        uint16_t token = cell.tokens[i];
        const FloorFace *face = contact_face(grid, token);
        FloorPlane plane = face->e[0];
        const FloorPoint *vertex = &grid->verts[plane.v];
        int n[] = {(int16_t)vertex->x, (int16_t)vertex->y, (int16_t)vertex->z};
        int clearance = n[1] < -3584 ? 6144 : 12032;
        if (crossing_margin(grid, face, (token & 0x8000) ? 4 : 3, from, end) < -clearance)
            continue;
        if (n[1] >= -0xbcb || (face->flags && !(road &&
            (face->flags == 0x2000 || face->flags == 0x4000 || face->flags == 2)))) return -1;
        int64_t penetration = -signed_distance(grid, plane, end);
        int64_t length = (int64_t)n[0] * n[0] + (int64_t)n[1] * n[1] + (int64_t)n[2] * n[2];
        int64_t projected[3];
        for (unsigned axis = 0; axis < 3; ++axis)
            projected[axis] = end[axis] + penetration * n[axis] / length;
        /* Start three normal units out. Integer truncation at a shallow
         * angle may require more clearance to exit the face. */
        int64_t distance = grid->dist[plane.d];
        for (unsigned axis = 0; axis < 3; ++axis)
            distance += projected[axis] * n[axis];
        int64_t step = 3;
        if (distance < clearance) {
            int64_t minimum = ((clearance - distance) * 4096 + length - 1) / length;
            if (minimum > step) step = minimum;
        }
        for (;; ++step) {
            for (unsigned axis = 0; axis < 3; ++axis) {
                int64_t value = projected[axis] + (int64_t)step * n[axis] / 4096;
                if (value < INT16_MIN || value > INT16_MAX) return -1;
                end[axis] = (int16_t)value;
            }
            if (signed_distance(grid, plane, end) >= clearance) break;
        }
        return 1;
    }
    return 0;
}

static int thrown_segment(const BfmForestField *field, const int16_t from[3],
                          int16_t to[3], int *grounded, int road)
{
    if (!field || !from || !to || !grounded || abs(to[0] - from[0]) >= 128 ||
        abs(to[2] - from[2]) >= 128) return 0;
    const FloorGrid *grid = &field->grid;
    int16_t end[3];
    memcpy(end, to, sizeof end);
    ContactCell start = contact_cell(grid, from);
    if (!start.count) return 0;
    int touched = 0;
    for (unsigned pass = 0; pass < 6; ++pass) {
        int cross_x = !same_grid_index(from[0], end[0]);
        int cross_z = !same_grid_index(from[2], end[2]);
        if (cross_x && cross_z) return 0;
        int hit = thrown_ground_cell(grid, start, from, end, road);
        if (!hit && (cross_x || cross_z)) {
            ContactCell finish = contact_cell(grid, end);
            if (!finish.count) return 0;
            hit = thrown_ground_cell(grid, finish, from, end, road);
        }
        if (hit < 0) return 0;
        if (!hit) {
            memcpy(to, end, sizeof end);
            *grounded = touched;
            return 1;
        }
        touched = 1;
    }
    return 0;
}

/* A resting target follows the bank with a travel-length floor sweep,
 * then a short support probe. An airborne target tests its exact path. */
static int body_ground_probe(const BfmForestField *field, const int16_t from[3],
                            int16_t to[3], int follow_ground, int *grounded, int road)
{
    if (!follow_ground) return thrown_segment(field, from, to, grounded, road);
    if (!from || !to || !grounded) return 0;
    int span = floor_sweep_span(from, to);
    if (from[1] < INT16_MIN + 4 || to[1] > INT16_MAX - span - 1) return 0;
    int16_t near[3] = {from[0], (int16_t)(from[1] - 4), from[2]};
    int16_t far[3] = {to[0], (int16_t)(to[1] + span + 1), to[2]};
    int hit;
    if (!thrown_segment(field, near, far, &hit, road) || !hit || far[1] > INT16_MAX - 6) return 0;
    int16_t support[3] = {far[0], (int16_t)(far[1] + 6), far[2]};
    if (!thrown_segment(field, far, support, &hit, road) || !hit) return 0;
    /* The support probe owns height only. */
    far[1] = support[1];
    memcpy(to, far, sizeof far);
    *grounded = 1;
    return 1;
}

int bfm_forest_thrown_probe(const BfmForestField *field, const int16_t from[3],
                            int16_t to[3], int follow_ground, int *grounded)
{
    return body_ground_probe(field, from, to, follow_ground, grounded, 0);
}

int bfm_forest_escape_head_probe(const BfmForestField *field, const int16_t from[3],
                                 int16_t to[3], int follow_ground, int *grounded)
{
    return field && field->room == SUMMIT_ROOM &&
           body_ground_probe(field, from, to, follow_ground, grounded, 1);
}

/* Rifle rounds test the segment against the room faces listed in the
 * starting cell. A solid plane preserves tangential motion and separates
 * the endpoint from the face in whole world units. */
static int separate_rifle_endpoint(const FloorGrid *grid, FloorPlane plane,
                                   int clearance, int16_t end[3])
{
    const FloorPoint *vector = &grid->verts[plane.v];
    const int n[3] = {(s16)vector->x, (s16)vector->y, (s16)vector->z};
    int64_t squared = (int64_t)n[0] * n[0] + (int64_t)n[1] * n[1] + (int64_t)n[2] * n[2];
    if (!squared) return 0;
    int64_t depth = -signed_distance(grid, plane, end);
    int surface[3];
    int64_t projected = grid->dist[plane.d];
    for (unsigned i = 0; i < 3; ++i) {
        surface[i] = end[i] + (int)(depth * n[i] / squared);
        projected += (int64_t)surface[i] * n[i];
    }
    /* Each whole displacement rounds toward the plane. Start at the first
     * step that could clear it before this rounding. */
    int64_t needed = ((clearance - projected) * 4096 + squared - 1) / squared;
    for (int64_t step = needed > 3 ? needed : 3; ; ++step) {
        for (unsigned i = 0; i < 3; ++i) {
            int64_t coordinate = surface[i] + step * n[i] / 4096;
            if (coordinate < INT16_MIN || coordinate > INT16_MAX) return 0;
            end[i] = (int16_t)coordinate;
        }
        if (signed_distance(grid, plane, end) >= clearance)
            return 1;
    }
}

unsigned bfm_forest_resolve_solid_segment(const BfmForestField *field, const int16_t from[3],
                               const int16_t to[3], int16_t impact[3])
{
    if (!field || !from || !to) return 0;
    const FloorGrid *grid = &field->grid;
    ContactCell cell = contact_cell(grid, from);
    int16_t endpoint[3];
    memcpy(endpoint, to, sizeof(endpoint));
    unsigned flags = 0, material = 0;
    for (unsigned retry = 0; retry < 6; ++retry) {
        int corrected = 0;
        for (unsigned i = 0; i < cell.count; ++i) {
            u16 token = cell.tokens[i];
            const FloorFace *face = contact_face(grid, token);
            if (face->flags & 0x100) continue;
            int ny = (s16)grid->verts[face->e[0].v].y;
            unsigned sides = token & 0x8000 ? 4 : 3;
            int clearance = ny < -3584 ? 6144 : EDGE_TOLERANCE;
            if (crossing_margin(grid, face, sides, from, endpoint) < -clearance)
                continue;
            unsigned surface = face->flags & 255;
            if (face->flags & 0x200) {
                material = surface;
                continue;
            }
            if (!separate_rifle_endpoint(grid, face->e[0], clearance, endpoint)) {
                if (impact) memcpy(impact, from, sizeof(endpoint));
                return 0x2000 | material;
            }
            if (ny < -3019) {
                flags |= 0x2000;
                material = surface;
            } else {
                flags = (flags & 0x2000) | (ny < -1400 ? 0x4000u : 0x8000u);
                if (surface) material = surface;
            }
            corrected = 1;
            break;
        }
        if (!corrected) break;
        if (retry == 5) {
            memcpy(endpoint, from, sizeof(endpoint));
            flags = 0x2000;
        }
    }
    if (material == 2 || material == 27) flags |= 0x1000;
    if (impact && (flags || material)) memcpy(impact, endpoint, sizeof(endpoint));
    return flags | material;
}

unsigned bfm_forest_rifle_wall(const BfmForestField *field, const int16_t from[3],
                               const int16_t to[3], int16_t impact[3])
{
    return bfm_forest_resolve_solid_segment(field, from, to, impact);
}
