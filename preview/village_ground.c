/* SPDX-License-Identifier: MIT */
#include "village_ground.h"
#include "forest_bank_thorns.h"

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { CELL, DISTANCE, NORMAL, LIST, TRIANGLE, QUAD, TABLE_COUNT };
enum { GENTLE_CLEARANCE = 6144, EDGE_CLEARANCE = 12032 };

typedef struct {
    const uint8_t *data;
    size_t bytes;
} Slice;

typedef struct {
    int normal[3];
    int32_t bias;
} Plane;

typedef struct {
    Plane planes[5];
    unsigned count, material;
} Face;

struct BfmVillageGround {
    Slice tables[TABLE_COUNT];
    int origin_x, origin_z, width, depth;
    unsigned exit_material, passage_wall;
    int lower_village, recheck_bank_floor, summit_contacts;
};

static unsigned read16(const uint8_t *p)
{
    return p[0] | (unsigned)p[1] << 8;
}

static uint32_t read32(const uint8_t *p)
{
    return read16(p) | (uint32_t)read16(p + 2) << 16;
}

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t need)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (!scene || address < base || (address & 3)) return NULL;
    size_t offset = address - base;
    return offset <= bytes && need <= bytes - offset ? scene + offset : NULL;
}

BfmVillageGround *bfm_village_ground_open(const uint8_t *scene, size_t bytes, uint32_t room)
{
    const uint8_t *descriptor = mounted(scene, bytes, room, 0x8c);
    if (!descriptor) return NULL;
    const uint8_t *grid = mounted(scene, bytes, read32(descriptor + 0x88), 32);
    if (!grid) return NULL;
    size_t end = bytes - (size_t)(grid - scene);
    BfmVillageGround view = {0};
    for (int i = TABLE_COUNT - 1; i >= 0; --i) {
        size_t offset = read32(grid + 8 + 4*i);
        if (offset < 32 || offset > end) return NULL;
        view.tables[i] = (Slice){grid + offset, end - offset};
        end = offset;
    }
    if (room == UINT32_C(0x8018c794)) view.exit_material = 0x106;
    view.summit_contacts = room == UINT32_C(0x801961e8);
    view.recheck_bank_floor = room == UINT32_C(0x8018ea38);
    if (room == UINT32_C(0x80192b18)) {
        view.exit_material = 0x105;
        view.lower_village = 1;
    }
    if (room == UINT32_C(0x8018ea38) || room == UINT32_C(0x801910e4) ||
        room == UINT32_C(0x801961e8) || room == UINT32_C(0x8018c890) ||
        room == UINT32_C(0x80186620))
        view.passage_wall = 0x203;
    view.origin_x = read16(grid);
    view.origin_z = read16(grid + 2);
    view.width = read16(grid + 4);
    view.depth = read16(grid + 6);
    if (!view.width || !view.depth || view.width > 512 || view.depth > 512 ||
        (size_t)view.width * view.depth * 4 > view.tables[CELL].bytes ||
        view.tables[DISTANCE].bytes % 4 || view.tables[NORMAL].bytes % 8 ||
        view.tables[LIST].bytes % 2 || view.tables[TRIANGLE].bytes % 18) return NULL;
    BfmVillageGround *ground = malloc(sizeof(*ground));
    if (ground) *ground = view;
    return ground;
}

void bfm_village_ground_free(BfmVillageGround *ground)
{
    free(ground);
}

static int cell(const BfmVillageGround *g, const int point[3], Slice *tokens)
{
    int x = (point[0] + 32768) / 128 - g->origin_x;
    int z = (point[2] + 32768) / 128 - g->origin_z;
    if (x < 0 || z < 0 || x >= g->width || z >= g->depth) return 0;
    const uint8_t *p = g->tables[CELL].data + 4 * (x + z*g->width);
    unsigned offset = read16(p), bytes = read16(p + 2) * 2;
    Slice list = g->tables[LIST];
    if (!bytes || offset % 2 || offset > list.bytes || bytes > list.bytes - offset) return 0;
    *tokens = (Slice){list.data + offset, bytes};
    return 1;
}

static int read_face(const BfmVillageGround *g, unsigned token, Face *face)
{
    unsigned quad = token >> 15, index = token & 0x7fff;
    unsigned stride = quad ? 22 : 18;
    Slice faces = g->tables[quad ? QUAD : TRIANGLE];
    if (index >= faces.bytes / stride) return 0;
    const uint8_t *p = faces.data + index * stride;
    face->count = quad ? 5 : 4;
    face->material = read16(p);
    for (unsigned i = 0; i < face->count; ++i) {
        unsigned ni = read16(p + 2 + 4*i), di = read16(p + 4 + 4*i);
        if (ni >= g->tables[NORMAL].bytes/8 || di >= g->tables[DISTANCE].bytes/4) return 0;
        const uint8_t *n = g->tables[NORMAL].data + 8*ni;
        Plane *plane = &face->planes[i];
        for (int k = 0; k < 3; ++k) {
            int v = (int)read16(n + 2*k);
            plane->normal[k] = v >= 32768 ? v - 65536 : v;
            if (abs(plane->normal[k]) > 4096) return 0;
        }
        uint32_t bias = read32(g->tables[DISTANCE].data + 4*di);
        plane->bias = (int32_t)((int64_t)bias - (bias > INT32_MAX ? INT64_C(4294967296) : 0));
    }
    return 1;
}

static int64_t distance(const Plane *plane, const int point[3])
{
    return plane->bias + (int64_t)point[0]*plane->normal[0] +
        (int64_t)point[1]*plane->normal[1] + (int64_t)point[2]*plane->normal[2];
}

static int clearance(const Plane *plane)
{
    return plane->normal[1] < -3584 ? GENTLE_CLEARANCE : EDGE_CLEARANCE;
}

static int crosses(const Face *face, const int start[3], const int end[3])
{
    int64_t before = distance(face->planes, start), after = distance(face->planes, end);
    if (before < 0 || after >= 0) return 0;
    int at[3];
    for (int k = 0; k < 3; ++k)
        at[k] = start[k] + (int)((end[k] - start[k]) * before / (before - after));
    for (unsigned i = 1; i < face->count; ++i)
        if (distance(&face->planes[i], at) < -clearance(face->planes)) return 0;
    return 1;
}

/* Separate the whole-unit penetration from the outward normal correction. */
static int project(const Plane *plane, int end[3])
{
    const int *n = plane->normal;
    int squared = n[0]*n[0] + n[1]*n[1] + n[2]*n[2];
    if (!squared) return 0;
    int64_t penetration = -distance(plane, end), foot[3];
    for (int k = 0; k < 3; ++k)
        foot[k] = end[k] + penetration*n[k]/squared;
    for (int step = 3; step < 35; ++step) {
        for (int k = 0; k < 3; ++k) {
            int64_t v = foot[k] + step*n[k]/4096;
            if (v < INT16_MIN || v > INT16_MAX) return 0;
            end[k] = (int)v;
        }
        if (distance(plane, end) >= clearance(plane)) return 1;
    }
    return 0;
}

/* Later faces in the actor sweep see the corrected endpoint. */
static int floors(const BfmVillageGround *g, const int start[3], int end[3],
                  Face *support, int bank_events)
{
    Slice list;
    if (!cell(g, start, &list)) return -1;
    int found = 0;
    for (size_t i = 0; i < list.bytes; i += 2) {
        Face face;
        if (!read_face(g, read16(list.data + i), &face)) return -1;
        /* Castle and forest thresholds carry the exit material on the walkable floor. */
        int threshold = g->lower_village && (face.material == 0x107 ||
                                             face.material == 0x109 || face.material == 0x10a);
        int thorn = bank_events && (uint8_t)face.material == BFM_FOREST_BANK_THORN_REASON;
        if (face.planes[0].normal[1] >= -1024 ||
            ((face.material & 0x300) && face.material != g->exit_material && !threshold && !thorn) ||
            !crosses(&face, start, end)) continue;
        *support = face;
        if (!project(face.planes, end)) return -1;
        found = 1;
    }
    return found;
}

static int slide(const BfmVillageGround *g, const int start[3], int end[3], uint16_t *wall,
                 int *wall_hit)
{
    Slice origin;
    if (!cell(g, start, &origin)) return 0;
    for (unsigned pass = 0; pass < 6; ++pass) {
        Slice cells[2] = {origin, {0}};
        if (!cell(g, end, &cells[1])) return 0;
        unsigned count = cells[1].data == origin.data && cells[1].bytes == origin.bytes ? 1 : 2;
        int adjusted = 0, passage = 0;
        for (unsigned c = 0; c < count && !adjusted; ++c) {
            for (size_t i = 0; i < cells[c].bytes; i += 2) {
                Face face;
                if (!read_face(g, read16(cells[c].data + i), &face)) return 0;
                /* Cleft bank walls can lean slightly away from vertical. */
                int bank_wall = g->recheck_bank_floor && face.material == 0x100;
                if ((face.planes[0].normal[1] && !bank_wall) || !crosses(&face, start, end)) continue;
                /* The marked forest exits report a crossing while allowing
                 * the regular stride to reach the far side of the plane. */
                if (g->passage_wall && face.material == g->passage_wall) {
                    passage = 1;
                    continue;
                }
                if (!project(face.planes, end)) return 0;
                *wall_hit = 1;
                if (face.material & 31u) *wall = (uint16_t)face.material;
                adjusted = 1;
                break;
            }
        }
        if (!adjusted) {
            if (passage) *wall = (uint16_t)g->passage_wall;
            return 1;
        }
    }
    return 0;
}

static void extend_foot_probe(int start[3], int end[3])
{
    int dx = end[0] - start[0], dz = end[2] - start[2];
    int span = (int)hypot(dx, dz);
    if (span < 3) span = 4;
    else if (span < 5) ++span;
    start[1] -= 4;
    end[1] += span + 1;
}

static BfmVillageGroundResult sweep(const BfmVillageGround *g, const int16_t from[3],
                                    int16_t to[3], BfmForestContact *floor, int airborne,
                                    uint16_t *wall, int *wall_hit, uint16_t *reason)
{
    if (!g || !from || !to || !floor) return BFM_VILLAGE_GROUND_BLOCKED;
    int start[3] = {from[0], from[1], from[2]}, end[3] = {to[0], to[1], to[2]};
    if (!airborne) extend_foot_probe(start, end);
    const int probe_end_y = end[1];
    Face support = {0};
    int hit = floors(g, start, end, &support, reason != NULL);
    if (hit < 0) return BFM_VILLAGE_GROUND_BLOCKED;
    if (!hit && !airborne) end[1] = to[1];
    uint16_t touched = 0;
    int projected_wall = 0;
    if (!slide(g, start, end, &touched, &projected_wall)) return BFM_VILLAGE_GROUND_BLOCKED;
    /* At the Cleft Woods bank, sliding along a wall can move the end of a
     * missed foot sweep over the bank or riverbed. Test the corrected stride
     * before advancing the player below that surface. */
    if (g->recheck_bank_floor && projected_wall && !hit) {
        int probe[3] = {end[0], probe_end_y, end[2]};
        int found = floors(g, start, probe, &support, 0);
        if (found < 0) return BFM_VILLAGE_GROUND_BLOCKED;
        if (found && probe[0] == end[0] && probe[2] == end[2]) {
            hit = 1;
            end[1] = probe[1];
        }
    }
    if (reason) {
        /* The floor sweep completes before the wall pass. Only a new marked
         * contact replaces the pending surface event. */
        if (hit) bfm_forest_bank_latch_material(reason, support.material);
        bfm_forest_bank_latch_material(reason, touched);
    }
    if (wall) *wall = touched;
    if (wall_hit) *wall_hit = projected_wall;
    for (int k = 0; k < 3; ++k) to[k] = (int16_t)end[k];
    if (!hit) return BFM_VILLAGE_GROUND_AIR;
    const int *n = support.planes[0].normal;
    *floor = (BfmForestContact){.height = to[1],
        .normal = {(int16_t)n[0], (int16_t)n[1], (int16_t)n[2]},
        .flags = (uint16_t)support.material,
        .slope = n[1] < -3019 ? 0x2000 : n[1] < -1400 ? 0x4000 : 0x8000};
    if (wall_hit && floor->slope == 0x8000) *wall_hit = 1;
    return BFM_VILLAGE_GROUND_FOOTING;
}

BfmVillageGroundResult bfm_village_ground_step(const BfmVillageGround *g, const int16_t from[3],
                                              int16_t to[3], BfmForestContact *floor)
{
    return sweep(g, from, to, floor, 0, NULL, NULL, NULL);
}

BfmVillageGroundResult bfm_village_ground_wall_step(const BfmVillageGround *g,
    const int16_t from[3], int16_t to[3], BfmForestContact *floor, uint16_t *wall)
{
    if (!g || !g->summit_contacts || !from || !to || !floor)
        return sweep(g, from, to, floor, 0, wall, NULL, NULL);

    int16_t proposed[3];
    memcpy(proposed, to, sizeof proposed);
    uint16_t marked = 0;
    BfmVillageGroundResult direct = sweep(g, from, to, floor, 0, &marked, NULL, NULL);
    if (wall) *wall = marked;
    if (marked || direct == BFM_VILLAGE_GROUND_BLOCKED) return direct;

    /* The upper walkway has slanted retaining faces. At a shared edge the
     * foot meets both a floor and a wall. Apply their successive projections
     * in the mounted cell's order before looking up the supporting normal. */
    uint16_t material;
    unsigned contact = bfm_village_ground_support_probe(g, 0, from, proposed, &material);
    if (!(contact & 0x2000) || !(contact & 0x8000)) return direct;
    int16_t supported[3];
    memcpy(supported, proposed, sizeof supported);
    BfmForestContact corrected;
    if (sweep(g, proposed, supported, &corrected, 0, NULL, NULL, NULL) != BFM_VILLAGE_GROUND_FOOTING)
        return direct;
    memcpy(to, supported, sizeof supported);
    *floor = corrected;
    return BFM_VILLAGE_GROUND_FOOTING;
}

BfmVillageGroundResult bfm_village_ground_air_contact(const BfmVillageGround *g,
    const int16_t from[3], int16_t to[3], BfmForestContact *floor, int *wall_hit)
{
    if (g && g->summit_contacts && from && to && floor) {
        /* A falling player can cross a slightly tilted retaining wall above
         * the lower floor. It corrects the swept foot without ending the fall. */
        int16_t foot[3];
        memcpy(foot, to, sizeof foot);
        uint16_t material;
        unsigned contact = bfm_village_ground_support_probe(g, 1, from, foot, &material);
        if (contact == (0x2000 | 0x8000)) {
            /* The steep face can push a descending foot back over the lip.
             * Confirm its supporting floor at that corrected position. */
            int16_t supported[3];
            memcpy(supported, foot, sizeof supported);
            BfmForestContact landing;
            if (sweep(g, foot, supported, &landing, 0, NULL, NULL, NULL) == BFM_VILLAGE_GROUND_FOOTING) {
                memcpy(to, supported, sizeof supported);
                *floor = landing;
                if (wall_hit) *wall_hit = 1;
                return BFM_VILLAGE_GROUND_FOOTING;
            }
        }
        if (contact == 0x8000) {
            memcpy(to, foot, sizeof foot);
            if (wall_hit) *wall_hit = 1;
            return BFM_VILLAGE_GROUND_AIR;
        }
    }
    return sweep(g, from, to, floor, 1, NULL, wall_hit, NULL);
}

BfmVillageGroundResult bfm_village_ground_air(const BfmVillageGround *g, const int16_t from[3],
                                             int16_t to[3], BfmForestContact *floor)
{
    return bfm_village_ground_air_contact(g, from, to, floor, NULL);
}

BfmVillageGroundResult bfm_village_ground_air_passage(const BfmVillageGround *g,
    const int16_t from[3], int16_t to[3], BfmForestContact *floor, uint16_t *wall)
{
    return sweep(g, from, to, floor, 1, wall, NULL, NULL);
}

BfmVillageGroundResult bfm_village_ground_bank_step(const BfmVillageGround *g,
    const int16_t from[3], int16_t to[3], int airborne, BfmForestContact *floor,
    uint16_t *wall, uint16_t *reason)
{
    if (!reason) return BFM_VILLAGE_GROUND_BLOCKED;
    return sweep(g, from, to, floor, airborne, wall, NULL, reason);
}

unsigned bfm_village_ground_support_probe(const BfmVillageGround *g, unsigned mode,
    const int16_t from[3], int16_t to[3], uint16_t *material)
{
    if (!g || !from || !to || !material || mode > 2) return 0;
    int start[3] = {from[0], from[1], from[2]};
    int end[3] = {to[0], to[1], to[2]};
    int single = mode == 2 || (!mode && !memcmp(from, to, 3 * sizeof(*to)));
    if (!mode) extend_foot_probe(start, end);
    if (mode == 2) {
        memcpy(end, start, sizeof end);
        end[1] += 6;
    }
    *material = 0;
    Slice list;
    if (!cell(g, start, &list)) return 0;
    unsigned gentle = 0, steep = 0;
    for (unsigned pass = 0; pass < 6; ++pass) {
        int corrected = 0;
        for (size_t i = 0; i < list.bytes; i += 2) {
            Face face;
            if (!read_face(g, read16(list.data + i), &face)) return 0;
            if (!crosses(&face, start, end)) continue;
            if (face.material & 0x200) {
                /* A marked passage reports its surface without supporting or
                 * projecting the foot. Remaining faces may still supply ground. */
                *material = (uint16_t)face.material;
                continue;
            }
            if (!project(face.planes, end)) return 0;
            int ny = face.planes[0].normal[1];
            if (ny < -3019) {
                gentle = 0x2000;
                *material = (uint16_t)face.material;
            } else {
                steep = ny < -1400 ? 0x4000 : 0x8000;
                if ((uint8_t)face.material) *material = (uint16_t)face.material;
            }
            corrected = 1;
            break;
        }
        if (!corrected || single) break;
        if (pass == 5) {
            memcpy(end, start, sizeof end);
            gentle = 0x2000;
            steep = 0;
        }
    }
    if (gentle || steep)
        for (int k = 0; k < 3; ++k) to[k] = (int16_t)end[k];
    return gentle | steep;
}

int bfm_village_ground_clear_floor(const BfmVillageGround *g, const int16_t from[3],
                                   int16_t to[3], int vertical_probe, BfmForestContact *floor)
{
    uint16_t material;
    int wall_hit;
    return sweep(g, from, to, floor, vertical_probe, &material, &wall_hit, NULL) == BFM_VILLAGE_GROUND_FOOTING &&
           !wall_hit && !material && floor->slope == 0x2000;
}

/* The blade stops just short of the contact along its direction of travel.
 * The reciprocal length is quantized before each signed component is rounded down. */
static void back_up_blade(const int delta[3], int point[3])
{
    unsigned squared = 0;
    for (int k = 0; k < 3; ++k) squared += (unsigned)(delta[k]*delta[k]);
    if (!squared) return;
    int divisor = 8;
    while (squared > 255) { squared >>= 2; divisor *= 2; }
    while (squared < 64) { squared <<= 2; divisor /= 2; }
    int gain = (int)sqrt((UINT32_C(1) << 30) / squared);
    int unit = divisor * 2048;
    for (int k = 0; k < 3; ++k) {
        int component = delta[k]*gain;
        point[k] -= component/unit - (component%unit < 0);
    }
}

static int blade_face(const Face *face, const int start[3], const int end[3],
                      const int delta[3], int point[3])
{
    const Plane *plane = face->planes;
    if ((face->material & 0x300) || (int32_t)distance(plane,end) >= 0) return 0;
    int32_t before = (int32_t)distance(plane,start);
    int slope = 0;
    for (int k = 0; k < 3; ++k) slope += plane->normal[k]*delta[k];
    if (before < 0 || !slope) return 0;
    for (int k = 0; k < 3; ++k) {
        /* This product occupies one signed word before division. */
        int32_t numerator = (int32_t)(-(int64_t)before * delta[k]);
        point[k] = (int16_t)(start[k] + (int64_t)numerator / slope);
    }
    for (unsigned i = 1; i < face->count; ++i)
        if ((int32_t)distance(&face->planes[i],point) < -EDGE_CLEARANCE) return 0;
    return 1;
}

int bfm_village_ground_blade(const BfmVillageGround *g, const int16_t from[3],
                            int16_t to[3], int16_t normal[3], uint16_t *material)
{
    if (!g || !from || !to || !normal || !material) return 0;
    int start[3], end[3], delta[3];
    for (int k = 0; k < 3; ++k) {
        start[k] = from[k];
        end[k] = to[k];
        delta[k] = (int16_t)(end[k] - start[k]);
    }
    int same_cell = ((uint16_t)from[0] >> 7) == ((uint16_t)to[0] >> 7) &&
                    ((uint16_t)from[2] >> 7) == ((uint16_t)to[2] >> 7);
    const int *cells[2] = {start, end};
    for (int c = 0; c < (same_cell ? 1 : 2); ++c) {
        Slice tokens;
        if (!cell(g,cells[c],&tokens)) continue;
        /* Each cell's last listed polygon takes precedence over earlier entries. */
        for (size_t remaining = tokens.bytes; remaining; remaining -= 2) {
            Face face;
            if (!read_face(g,read16(tokens.data + remaining - 2),&face)) return 0;
            int point[3];
            if (!blade_face(&face,start,end,delta,point)) continue;
            back_up_blade(delta,point);
            for (int k = 0; k < 3; ++k) {
                to[k] = (int16_t)point[k];
                normal[k] = (int16_t)face.planes[0].normal[k];
            }
            *material = (uint16_t)face.material;
            return 1;
        }
    }
    return 0;
}
