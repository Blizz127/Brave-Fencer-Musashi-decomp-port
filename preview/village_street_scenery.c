/* SPDX-License-Identifier: MIT */
#include "village_street_scenery.h"
#include "forest_pose.h"
#include "forest_raster.h"
#include "../pc_port/forest_player_bytes.h"

#include <limits.h>
#include <math.h>
#include <stdlib.h>

enum { TREE_LIMIT = 16, ROOF_VERTICES = 26, ROOF_FACES = 24, BELL_VERTICES = 19, BELL_FACES = 18, CORNERS = 4 };
static const uint32_t scene_origin = UINT32_C(0x80128158);

typedef struct {
    BfmForestRasterVertex points[CORNERS];
    BfmForestRasterMaterial material;
    int depth, with_offset;
} StreetFace;

typedef struct {
    MusashiForestPlayerMesh *mesh;
    int16_t center[3];
    unsigned heading, ticks;
    enum { WAITING, GAINING_SPEED, COASTING, SLOWING } phase;
    uint32_t angle_q16;
    int speed_q16, acceleration_q16;
} WeatherVane;

struct BfmVillageStreetScenery {
    unsigned count, visible, next;
    int16_t roots[TREE_LIMIT][3];
    MusashiForestPlayerVector local[CORNERS];
    BfmForestRasterMaterial material;
    BfmForestRasterVertex texture[CORNERS];
    MusashiForestPlayerMesh *roof, *bell;
    int16_t hanger[3];
    WeatherVane vane;
    unsigned tick, progress, draw_page;
    int ambient[3], daylight[3][3];
    StreetFace faces[TREE_LIMIT + ROOF_FACES + BELL_FACES];
    BfmForestRaster raster;
};

static const uint8_t *scene_at(const uint8_t *scene, size_t bytes, uint32_t addr, size_t need)
{
    if (addr < scene_origin || addr - scene_origin > bytes ||
        need > bytes - (addr - scene_origin)) return NULL;
    return scene + (addr - scene_origin);
}

static int read_roots(BfmVillageStreetScenery *scenery, const uint8_t *scene,
                      size_t bytes, uint32_t list)
{
    for (unsigned i = 0; i < TREE_LIMIT; ++i) {
        const uint8_t *row = scene_at(scene, bytes, list, 2);
        if (!row) return 0;
        if (s16le(row) == INT16_MIN) return i != 0;
        if (!scene_at(scene, bytes, list, 6)) return 0;
        for (unsigned axis = 0; axis < 3; ++axis)
            scenery->roots[i][axis] = s16le(row + 2 * axis);
        ++scenery->count;
        list += 6;
    }
    return 1;
}

static int read_street(BfmVillageStreetScenery *scenery, const uint8_t *scene,
                       size_t bytes, uint32_t room)
{
    const uint8_t *descriptor = scene_at(scene, bytes, room, 0xbc);
    if (!descriptor) return 0;
    uint32_t first = u32le(descriptor + 0xb4), end = u32le(descriptor + 0xb8);
    if (end < first || (end - first) % 20) return 0;
    const uint8_t *records = scene_at(scene, bytes, first, end - first);
    if (!records) return 0;
    int roof = 0;
    for (size_t offset = 0; offset < end - first; offset += 20) {
        const uint8_t *p = records + offset;
        unsigned kind = u16le(p + 6);
        if (!kind) break;
        if (kind == 298) roof = 1;
        if (kind == 106 && u16le(p + 8) == 1) {
            if (scenery->count || !read_roots(scenery, scene, bytes, u32le(p + 16))) return 0;
        }
    }
    if (!scenery->count) return 0;
    /* The street controller places the church belfry at the scene origin. Its
     * roof is in field coordinates. The bell has a separate hanger and swing. */
    if (roof) {
        const uint8_t *hanger = scene_at(scene, bytes, UINT32_C(0x80191948), 6);
        const uint8_t *vane = scene_at(scene, bytes, UINT32_C(0x80191958), 8);
        const uint8_t *daylight = scene_at(scene, bytes, UINT32_C(0x8018de3c) + 9, 9);
        if (!hanger || !vane || !daylight) return 0;
        scenery->vane.heading = u16le(vane + 6);
        for (unsigned k = 0; k < 3; ++k) {
            scenery->vane.center[k] = s16le(vane + 2*k);
            scenery->hanger[k] = s16le(hanger + 2*k);
            scenery->ambient[k] = s16le(descriptor + 2*k) & ~15;
            for (unsigned i = 0; i < 3; ++i)
                scenery->daylight[i][k] = daylight[3*i+k]*4096/255;
        }
        unsigned count;
        scenery->vane.mesh = musashi_forest_player_mesh_from_table(scene, bytes, scene_origin,
                                                                  UINT32_C(0x801bee78), &count);
        if (!scenery->vane.mesh || count != 1) return 0;
        scenery->bell = musashi_forest_player_mesh_from_table(scene, bytes, scene_origin,
                                                              UINT32_C(0x801be978), &count);
        const MusashiForestPlayerPart *bell = musashi_forest_player_mesh_part(scenery->bell, 0);
        if (!bell || count != 1 || bell->face_count != BELL_FACES ||
            bell->normal_count != BELL_VERTICES || bell->vertex_count != BELL_VERTICES) return 0;
        for (unsigned i = 0; i < BELL_FACES; ++i) {
            const MusashiForestPlayerFace *f = bell->faces + i;
            if (f->mode != 0x38 && f->mode != 0x30) return 0;
            for (unsigned k = 0; k < 3; ++k)
                if (f->rgb[k] != bell->faces[0].rgb[k]) return 0;
        }
        scenery->roof = musashi_forest_player_mesh_from_table(scene, bytes, scene_origin,
                                                             UINT32_C(0x801ca118), &count);
        const MusashiForestPlayerPart *part = musashi_forest_player_mesh_part(scenery->roof, 0);
        if (!part || count != 1 || part->face_count != ROOF_FACES ||
            part->vertex_count != ROOF_VERTICES) return 0;
        for (unsigned i = 0; i < ROOF_FACES; ++i) {
            const MusashiForestPlayerFace *f = part->faces + i;
            if ((f->mode != 0x24 && f->mode != 0x2c) || ((f->tpage >> 7) & 3u) != 1) return 0;
        }
    }
    return 1;
}

BfmVillageStreetScenery *bfm_village_street_scenery_load(const uint8_t *scene,
                                                        size_t bytes, uint32_t room)
{
    if (!scene || bytes > BFM_FIELD_SCENE_CAPACITY) return NULL;
    BfmVillageStreetScenery *scenery = calloc(1, sizeof(*scenery));
    if (!scenery) return NULL;
    if (!read_street(scenery, scene, bytes, room)) {
        bfm_village_street_scenery_free(scenery);
        return NULL;
    }

    /* All the placed trees share one camera-facing quad from the village bank. */
    unsigned count;
    MusashiForestPlayerMesh *model = musashi_forest_player_mesh_from_table(scene, bytes,
                                        scene_origin, UINT32_C(0x80190f74), &count);
    const MusashiForestPlayerPart *part = model ? musashi_forest_player_mesh_part(model, 0) : NULL;
    if (!part || count != 1 || part->face_count != 1 || part->faces->mode != 0x2d ||
        part->faces->corners != CORNERS || ((part->faces->tpage >> 7) & 3u) > 2) {
        musashi_forest_player_mesh_free(model);
        bfm_village_street_scenery_free(scenery);
        return NULL;
    }
    const MusashiForestPlayerFace *face = part->faces;
    scenery->material = (BfmForestRasterMaterial){.page = face->tpage, .clut = face->clut,
                                                  .count = CORNERS, .textured = 1};
    for (unsigned i = 0; i < CORNERS; ++i) {
        scenery->local[i] = part->vertices[face->vertex[i]];
        scenery->texture[i] = (BfmForestRasterVertex){.u = face->u[i], .v = face->v[i],
                                                       .rgb = 0x808080};
    }
    musashi_forest_player_mesh_free(model);
    return scenery;
}

void bfm_village_street_scenery_free(BfmVillageStreetScenery *scenery)
{
    if (scenery) {
        musashi_forest_player_mesh_free(scenery->vane.mesh);
        musashi_forest_player_mesh_free(scenery->roof);
        musashi_forest_player_mesh_free(scenery->bell);
    }
    free(scenery);
}

static int floor_q12(int64_t value)
{
    return (int)(value / 4096 - (value % 4096 < 0));
}

static void enqueue(BfmVillageStreetScenery *scenery, const StreetFace *face)
{
    unsigned slot = scenery->visible++;
    while (slot && scenery->faces[slot - 1].depth <= face->depth) {
        scenery->faces[slot] = scenery->faces[slot - 1];
        --slot;
    }
    scenery->faces[slot] = *face;
}

void bfm_village_street_scenery_step(BfmVillageStreetScenery *scenery,
                                     unsigned scene_tick, unsigned progress)
{
    if (scenery) {
        scenery->tick = scene_tick;
        scenery->progress = progress;
    }
}

void bfm_village_street_vane_step(BfmVillageStreetScenery *scenery, BfmGameplayRun *run)
{
    WeatherVane *vane = &scenery->vane;
    if (!vane->mesh || !run) return;
    if (vane->phase != WAITING) {
        if (vane->phase != COASTING) vane->speed_q16 += vane->acceleration_q16;
        /* The high half of the accumulated angle drives the model's yaw. */
        vane->angle_q16 += (uint32_t)vane->speed_q16;
        vane->heading = vane->angle_q16 >> 16;
    }
    if (vane->ticks && --vane->ticks) return;
    switch (vane->phase) {
    case WAITING:
        vane->acceleration_q16 = 30720 + (bfm_gameplay_random(run) & 16383);
        if (!(bfm_gameplay_random(run) & 1)) vane->acceleration_q16 = -vane->acceleration_q16;
        vane->speed_q16 = 0;
        vane->ticks = 64;
        vane->phase = GAINING_SPEED;
        break;
    case GAINING_SPEED:
        vane->ticks = 64 + (bfm_gameplay_random(run) & 63);
        vane->phase = COASTING;
        break;
    case COASTING:
        vane->acceleration_q16 = -vane->acceleration_q16;
        vane->ticks = 64;
        vane->phase = SLOWING;
        break;
    case SLOWING:
        vane->ticks = 64 + (bfm_gameplay_random(run) & 255);
        vane->phase = WAITING;
        break;
    }
}

int bfm_village_street_vane_add(const BfmVillageStreetScenery *scenery, BfmBedroomFigure *figure,
                               const BfmBedroomView *view)
{
    const WeatherVane *vane = &scenery->vane;
    if (!vane->mesh) return 1;
    static const BfmForestPose pose = {.local = {{.rotation = {{4096,0,0},{0,4096,0},{0,0,4096}}}}};
    BfmForestTransform root;
    bfm_forest_actor_root(vane->heading, vane->center, &root);
    const BfmForestActor actor = {.root = &root, .mesh = vane->mesh, .pose = &pose, .visible_parts = 1};
    return bfm_bedroom_figure_add_actor(figure, view, &actor, NULL, NULL, 0);
}

static void rotated(const BfmForestTransform *rotation, const MusashiForestPlayerVector *v,
                    int result[3])
{
    for (unsigned k = 0; k < 3; ++k) {
        const int16_t *row = rotation->rotation[k];
        result[k] = floor_q12((int64_t)row[0]*v->x + (int64_t)row[1]*v->y + (int64_t)row[2]*v->z);
    }
}

static uint32_t bell_color(const BfmVillageStreetScenery *scenery, const int normal[3],
                           const uint8_t rgb[3])
{
    /* The street is lit from above, from an incline and from below. */
    int light[3] = {-normal[1], floor_q12(-1365LL*normal[1] - 3900LL*normal[2]), normal[1]};
    uint32_t color = 0;
    for (unsigned k = 0; k < 3; ++k) {
        int64_t sum = 0;
        for (unsigned i = 0; i < 3; ++i)
            if (light[i] > 0) sum += scenery->daylight[i][k] * (int64_t)light[i];
        int v = (scenery->ambient[k] + floor_q12(sum)) * rgb[k] / 4096;
        if (v < 0) v = 0;
        if (v > 255) v = 255;
        color |= (uint32_t)v << (8*k);
    }
    return color;
}

static void prepare_landmark(BfmVillageStreetScenery *scenery, const BfmBedroomView *view, int bell)
{
    const MusashiForestPlayerPart *part =
        musashi_forest_player_mesh_part(bell ? scenery->bell : scenery->roof, 0);
    if (!part || (bell && scenery->progress >= 385 && scenery->progress < 470)) return;
    int projected[ROOF_VERTICES][3], valid[ROOF_VERTICES];
    uint32_t colors[BELL_VERTICES];
    BfmForestTransform rotation;
    if (bell) {
        const int16_t zero[3] = {0};
        BfmForestTransform wave;
        bfm_forest_actor_root(scenery->tick * 48u, zero, &wave);
        int sine = wave.rotation[0][2];
        int pitch = sine / 32 - (sine % 32 < 0);
        bfm_forest_actor_pitch_yaw((unsigned)pitch, 0, scenery->hanger, &rotation);
        for (unsigned i = 0; i < part->normal_count; ++i) {
            int normal[3];
            rotated(&rotation, part->normals + i, normal);
            colors[i] = bell_color(scenery, normal, part->faces[0].rgb);
        }
    }
    for (unsigned i = 0; i < part->vertex_count; ++i) {
        const MusashiForestPlayerVector *p = part->vertices + i;
        int16_t location[3] = {p->x, p->y, p->z};
        if (bell) {
            int point[3];
            rotated(&rotation, p, point);
            for (unsigned k = 0; k < 3; ++k) {
                int value = point[k] + scenery->hanger[k];
                if (value < INT16_MIN || value > INT16_MAX) return;
                location[k] = (int16_t)value;
            }
        }
        valid[i] = bfm_bedroom_project_packet(view, location, projected[i]);
    }
    for (unsigned i = 0; i < part->face_count; ++i) {
        const MusashiForestPlayerFace *f = part->faces + i;
        StreetFace face = {.material = {.page = f->tpage, .clut = f->clut, .count = f->corners,
                                        .textured = !bell, .dither = bell}, .with_offset = 1};
        int sum = 0;
        unsigned k;
        for (k = 0; k < f->corners; ++k) {
            const int *p = projected[f->vertex[k]];
            int x = p[0] - 160, y = p[1] - 120;
            if (!valid[f->vertex[k]]) break;
            face.points[k] = (BfmForestRasterVertex){.x = x, .y = y,
                                  .u = f->u[k], .v = f->v[k],
                                  .rgb = bell ? colors[f->normal[k]] : UINT32_C(0x808080)};
            sum += p[2];
        }
        if (k != f->corners) continue;
        /* The original culls in projection space. Across a drawing-offset
         * boundary, the surviving polygon can have reversed screen winding. */
        const BfmForestRasterVertex *p = face.points;
        if ((p[1].x-p[0].x)*(p[2].y-p[0].y) <= (p[1].y-p[0].y)*(p[2].x-p[0].x)) continue;
        face.depth = sum / (4 * (int)f->corners);
        enqueue(scenery, &face);
    }
}

int bfm_village_street_scenery_prepare(BfmVillageStreetScenery *scenery,
                                      const BfmBedroomView *view,
                                      const uint16_t tiles[512][1024], uint32_t *pixels, unsigned draw_page)
{
    if (!scenery || !view || !tiles || !pixels) return 0;
    scenery->raster = (BfmForestRaster){.tiles = tiles, .pixels = (uint32_t (*)[320])pixels,
                                        .top = 10, .bottom = 229};
    scenery->draw_page = draw_page;
    scenery->next = scenery->visible = 0;
    prepare_landmark(scenery, view, 0);
    prepare_landmark(scenery, view, 1);
    const double turn = 4096.0 / 6.28318530717958647693;
    int yaw = (int)lround(turn * atan2(-(double)view->axes[0][2], view->axes[0][0]));
    const int16_t center[3] = {0};
    BfmForestTransform tree;
    bfm_forest_actor_pitch_yaw((unsigned)-32, (unsigned)yaw, center, &tree);
    int offsets[CORNERS][3];
    for (unsigned k = 0; k < CORNERS; ++k) {
        MusashiForestPlayerVector v = scenery->local[k];
        for (unsigned axis = 0; axis < 3; ++axis) {
            /* Scale by 3 3/4 about the trunk before rotation into the street. */
            const int16_t *r = tree.rotation[axis];
            int64_t dot = (int64_t)r[0] * v.x + (int64_t)r[1] * v.y + (int64_t)r[2] * v.z;
            offsets[k][axis] = floor_q12(dot * 15 / 4);
        }
    }
    for (unsigned i = 0; i < scenery->count; ++i) {
        StreetFace face = {.material = scenery->material};
        int sum = 0;
        unsigned k;
        for (k = 0; k < CORNERS; ++k) {
            int16_t world[3];
            for (unsigned axis = 0; axis < 3; ++axis) {
                int coordinate = scenery->roots[i][axis] + offsets[k][axis];
                if (coordinate < INT16_MIN || coordinate > INT16_MAX) return 0;
                world[axis] = (int16_t)coordinate;
            }
            int projected[3];
            if (!bfm_bedroom_project(view, world, projected) || projected[2] <= view->focal / 2)
                break;
            face.points[k] = scenery->texture[k];
            face.points[k].x = projected[0];
            face.points[k].y = projected[1];
            sum += projected[2];
        }
        if (k != CORNERS) continue;
        face.depth = sum / 16;
        enqueue(scenery, &face);
    }
    return 1;
}

int bfm_village_street_scenery_next_depth(const BfmVillageStreetScenery *scenery)
{
    if (scenery && scenery->next < scenery->visible) return scenery->faces[scenery->next].depth;
    return INT_MIN;
}

void bfm_village_street_scenery_draw_to(BfmVillageStreetScenery *scenery, int depth)
{
    if (!scenery) return;
    while (scenery->next < scenery->visible && scenery->faces[scenery->next].depth >= depth) {
        const StreetFace *face = &scenery->faces[scenery->next++];
        if (face->with_offset)
            bfm_forest_raster_buffer_face(&scenery->raster, &face->material, face->points, scenery->draw_page);
        else
            bfm_forest_raster_face(&scenery->raster, &face->material, face->points);
    }
}
