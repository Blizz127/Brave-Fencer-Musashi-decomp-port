/* SPDX-License-Identifier: MIT */
#include "forest_shore_figures.h"
#include "forest_shore_pose.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>

enum { FIGURES = 2, BODY_PARTS = 24, CLIP_STEPS = 20 };

typedef struct {
    BfmForestTransform root;
    BfmForestPose pose;
    unsigned state, tick;
} ShoreFigure;

struct BfmForestShoreFigures {
    const BfmForestField *field;
    MusashiForestPlayerMesh *body, *base;
    BfmForestPose base_pose;
    ShoreFigure placed[FIGURES];
};

static void identity(BfmForestTransform *transform)
{
    for (unsigned axis = 0; axis < 3; ++axis)
        transform->rotation[axis][axis] = 4096;
}

static MusashiForestPlayerMesh *load_shape(const BfmForestField *field,
                                           uint32_t slot, unsigned parts)
{
    const uint8_t *reference = bfm_forest_room_bytes(field, slot, 4);
    if (!reference) return NULL;
    uint32_t table = u32le(reference);
    size_t table_bytes = 4 * (parts + 1);
    const uint8_t *list = bfm_forest_room_bytes(field, table, table_bytes);
    if (!list || u32le(list + 4 * parts)) return NULL;
    uint32_t begin = u32le(list);
    if (begin >= table || table - begin > (1u << 20)) return NULL;
    size_t span = (size_t)(table - begin) + table_bytes;
    const uint8_t *region = bfm_forest_room_bytes(field, begin, span);
    unsigned count = 0;
    MusashiForestPlayerMesh *mesh =
        musashi_forest_player_mesh_from_table(region, span, begin, table, &count);
    if (count == parts) return mesh;
    musashi_forest_player_mesh_free(mesh);
    return NULL;
}

BfmForestShoreFigures *bfm_forest_shore_figures_open(const BfmForestField *field)
{
    const uint8_t *placements = bfm_forest_room_bytes(field, UINT32_C(0x801b1a8c), 20 * FIGURES);
    if (!placements) return NULL;
    BfmForestShoreFigures *figures = calloc(1, sizeof(*figures));
    if (!figures) return NULL;
    figures->field = field;
    for (unsigned i = 0; i < FIGURES; ++i) {
        const uint8_t *record = placements + 20 * i;
        if (u16le(record + 6) != 12 || u16le(record + 8) || u16le(record + 12))
            goto failed;
        BfmForestTransform *root = &figures->placed[i].root;
        identity(root);
        for (unsigned axis = 0; axis < 3; ++axis)
            root->translation[axis] = s16le(record + 2 * axis);
    }
    figures->body = load_shape(field, UINT32_C(0x8018aebc), BODY_PARTS);
    figures->base = load_shape(field, UINT32_C(0x8018afa8), 1);
    if (!figures->body || !figures->base) goto failed;
    identity(&figures->base_pose.local[0]);
    return figures;
failed:
    bfm_forest_shore_figures_free(figures);
    return NULL;
}

void bfm_forest_shore_figures_free(BfmForestShoreFigures *figures)
{
    if (!figures) return;
    musashi_forest_player_mesh_free(figures->body);
    musashi_forest_player_mesh_free(figures->base);
    free(figures);
}

static int player_near(const int16_t position[3], const BfmForestTransform *root)
{
    int64_t distance = 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        int64_t delta = (int64_t)position[axis] - root->translation[axis];
        distance += delta * delta;
    }
    return distance < 384 * 384;
}

void bfm_forest_shore_figures_tick(BfmForestShoreFigures *figures, const int16_t position[3])
{
    if (!figures || !position) return;
    for (unsigned i = 0; i < FIGURES; ++i) {
        ShoreFigure *f = &figures->placed[i];
        unsigned state = f->state, tick = (f->tick + 1) % CLIP_STEPS;
        if (!state) {
            int dz = position[2] - f->root.translation[2];
            if (dz <= -768 || dz >= 768) continue;
            state = 1;
            tick = 0;
        } else if (state == 1 && player_near(position, &f->root)) {
            state = 2;
            tick = 0;
        } else if (state == 2 && !tick) {
            state = 3;
        }
        if (bfm_forest_shore_pose(figures->field, state, tick, &f->pose)) {
            f->state = state;
            f->tick = tick;
        }
    }
}

size_t bfm_forest_shore_figures_actors(const BfmForestShoreFigures *figures,
                                      BfmForestActor *out, size_t capacity)
{
    if (!figures || !out) return 0;
    size_t count = 0;
    for (unsigned i = 0; i < FIGURES; ++i)
        if (figures->placed[i].state) count += 2;
    if (capacity < count) return 0;
    size_t index = 0;
    for (unsigned i = 0; i < FIGURES; ++i) {
        const ShoreFigure *f = &figures->placed[i];
        if (!f->state) continue;
        out[index++] = (BfmForestActor){figures->body, &f->pose, &f->root,
                                       (UINT32_C(1) << BODY_PARTS) - 1};
        out[index++] = (BfmForestActor){figures->base, &figures->base_pose, &f->root, 1};
    }
    return count;
}
