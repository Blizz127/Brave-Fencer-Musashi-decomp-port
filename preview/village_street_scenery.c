/* SPDX-License-Identifier: MIT */
#include "village_street_scenery.h"
#include "forest_pose.h"
#include "forest_raster.h"
#include "../pc_port/forest_player_bytes.h"

#include <limits.h>
#include <math.h>
#include <stdlib.h>

enum { TREE_LIMIT = 16, CORNERS = 4 };
static const uint32_t scene_origin = UINT32_C(0x80128158);

typedef struct {
    BfmForestRasterVertex points[CORNERS];
    int depth;
} TreeFace;

struct BfmVillageStreetScenery {
    unsigned count, visible, next;
    int16_t roots[TREE_LIMIT][3];
    MusashiForestPlayerVector local[CORNERS];
    BfmForestRasterMaterial material;
    BfmForestRasterVertex texture[CORNERS];
    TreeFace faces[TREE_LIMIT];
    BfmForestRaster raster;
};

static const uint8_t *scene_at(const uint8_t *scene, size_t bytes, uint32_t addr, size_t need)
{
    if (addr < scene_origin || addr - scene_origin > bytes ||
        need > bytes - (addr - scene_origin)) return NULL;
    return scene + (addr - scene_origin);
}

static int read_roots(BfmVillageStreetScenery *scenery, const uint8_t *scene,
                      size_t bytes, uint32_t room)
{
    const uint8_t *descriptor = scene_at(scene, bytes, room, 0xbc);
    if (!descriptor) return 0;
    uint32_t first = u32le(descriptor + 0xb4), end = u32le(descriptor + 0xb8);
    if (end < first || (end - first) % 20) return 0;
    const uint8_t *records = scene_at(scene, bytes, first, end - first);
    if (!records) return 0;
    for (size_t offset = 0; offset < end - first; offset += 20) {
        const uint8_t *p = records + offset;
        if (!u16le(p + 6)) break;
        if (u16le(p + 6) != 106 || u16le(p + 8) != 1) continue;
        uint32_t list = u32le(p + 16);
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
    return 0;
}

BfmVillageStreetScenery *bfm_village_street_scenery_load(const uint8_t *scene,
                                                        size_t bytes, uint32_t room)
{
    if (!scene || bytes > BFM_FIELD_SCENE_CAPACITY) return NULL;
    BfmVillageStreetScenery *scenery = calloc(1, sizeof(*scenery));
    if (!scenery) return NULL;
    if (!read_roots(scenery, scene, bytes, room)) {
        free(scenery);
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
        free(scenery);
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
    free(scenery);
}

static int floor_q12(int64_t value)
{
    return (int)(value / 4096 - (value % 4096 < 0));
}

int bfm_village_street_scenery_prepare(BfmVillageStreetScenery *scenery,
                                      const BfmBedroomView *view,
                                      const uint16_t tiles[512][1024], uint32_t *pixels)
{
    if (!scenery || !view || !tiles || !pixels) return 0;
    scenery->raster = (BfmForestRaster){.tiles = tiles, .pixels = (uint32_t (*)[320])pixels,
                                        .top = 10, .bottom = 229};
    scenery->next = scenery->visible = 0;
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
        TreeFace face;
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
        unsigned slot = scenery->visible++;
        while (slot && scenery->faces[slot - 1].depth < face.depth) {
            scenery->faces[slot] = scenery->faces[slot - 1];
            --slot;
        }
        scenery->faces[slot] = face;
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
        bfm_forest_raster_face(&scenery->raster, &scenery->material,
                               scenery->faces[scenery->next++].points);
    }
}
