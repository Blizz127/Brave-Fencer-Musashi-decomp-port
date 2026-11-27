/* SPDX-License-Identifier: MIT */
#include "forest_sweep.h"

#include "forest_opening_data.h"
#include "../pc_port/forest_player_archive.h"
#include <limits.h>
#include <math.h>
#include <string.h>

static int64_t floor_bits(int64_t value, unsigned bits)
{
    int64_t unit = INT64_C(1) << bits;
    return value / unit - (value % unit < 0);
}

int bfm_forest_sweep_begin(BfmForestSweep *sweep, const BfmForestField *field)
{
    if (!sweep || !field) return 0;
    const uint8_t *timing = bfm_forest_room_bytes(field, UINT32_C(0x80189a80), 2);
    const uint8_t *length = bfm_forest_room_bytes(field, UINT32_C(0x80189ab0), 1);
    if (!timing || !length || !timing[0] || !timing[1] || !*length) return 0;
    *sweep = (BfmForestSweep){.delay = timing[0], .duration = timing[1],
                              .divisor = *length, .energy = 244};
    return 1;
}

int bfm_forest_sweep_edge(const BfmForestTransform *root, const int8_t anchors[2][3],
                          BfmForestSweepEdge *edge)
{
    if (!root || !anchors || !edge) return 0;
    for (unsigned i = 0; i < 2; ++i) {
        for (unsigned axis = 0; axis < 3; ++axis) {
            int32_t dot = 0;
            for (unsigned j = 0; j < 3; ++j) dot += root->rotation[axis][j] * anchors[i][j];
            int64_t v = (int64_t)root->translation[axis] + floor_bits(dot, 12);
            if (v < INT16_MIN || v > INT16_MAX) return 0;
            edge->point[i][axis] = (int16_t)v;
        }
    }
    return 1;
}

/* Each participating logical pose supplies its two signed blade anchors.
 * Save transformed points so old ribbon edges do not turn or move with the
 * current pose. */
static int sample_edge(const MusashiDiscMedia *disc, uint32_t pose,
                       const BfmForestTransform *root, BfmForestSweepEdge *edge)
{
    ForestPlayerArchive archive;
    int8_t anchors[2][3];
    if (!root || pose < 16 || !forest_player_archive_open(disc, &archive) ||
        !forest_player_archive_read(&archive, pose - 16, (uint8_t *)anchors, sizeof anchors))
        return 0;
    return bfm_forest_sweep_edge(root, anchors, edge);
}


static int curved_midpoint(const BfmForestSweepEdge *a, const BfmForestSweepEdge *b,
                           unsigned divisor, BfmForestSweepEdge *out)
{
    int axis[3];
    uint64_t length2 = 0;
    for (unsigned k = 0; k < 3; ++k) {
        int tip = (int)floor_bits(a->point[0][k] + b->point[0][k], 1);
        int hilt = (int)floor_bits(a->point[1][k] + b->point[1][k], 1);
        out->point[1][k] = (int16_t)hilt;
        axis[k] = tip - hilt;
        length2 += (uint64_t)((int64_t)axis[k] * axis[k]);
    }
    /* A fixed-point direction preserves the blade length between two poses
     * even when the edge turns sharply. The normalized square uses eight
     * significant bits; each two-bit shift changes the root scale by one. */
    int exponent = 3;
    if (length2) {
        while (length2 > 255) { length2 >>= 2; ++exponent; }
        while (length2 < 64) { length2 <<= 2; --exponent; }
    }
    int gain = length2 ? (int)sqrt((UINT32_C(1) << 30) / length2) : 0;
    for (unsigned k = 0; k < 3; ++k) {
        int64_t q = (int64_t)axis[k] * gain;
        int direction = (int)floor_bits(q, (unsigned)exponent);
        int tip = out->point[1][k] + direction / (int)divisor;
        if (tip < INT16_MIN || tip > INT16_MAX) return 0;
        out->point[0][k] = (int16_t)tip;
    }
    return 1;
}

static void prepend(BfmForestSweep *sweep, BfmForestSweepEdge edge)
{
    if (sweep->count <= BFM_FOREST_SWEEP_CARDS) ++sweep->count;
    memmove(sweep->edges + 1, sweep->edges, (sweep->count - 1) * sizeof(edge));
    sweep->edges[0] = edge;
}

static void fade(BfmForestSweep *sweep)
{
    sweep->duration = 0;
    if (sweep->energy < 32) {
        sweep->energy = sweep->count = 0;
        return;
    }
    sweep->energy -= 16;
    if (sweep->count > 1 && sweep->energy / 16 < sweep->count - 1) --sweep->count;
}

int bfm_forest_sweep_append(BfmForestSweep *sweep, const BfmForestSweepEdge *edge)
{
    if (!sweep || !sweep->divisor) return 0;
    if (!edge) {
        if (sweep->energy) fade(sweep);
        return 1;
    }
    if (sweep->count) {
        BfmForestSweepEdge middle;
        if (!curved_midpoint(&sweep->edges[0], edge, sweep->divisor, &middle)) return 0;
        prepend(sweep, middle);
    }
    prepend(sweep, *edge);
    return 1;
}

int bfm_forest_sweep_step(BfmForestSweep *sweep, const MusashiDiscMedia *disc,
                          uint32_t logical_pose, const BfmForestTransform *model)
{
    if (!sweep) return 0;
    if (!sweep->energy) return 1;
    if (sweep->delay && !logical_pose) {
        sweep->energy = 0;
        return 1;
    }
    if (sweep->delay > 1) {
        --sweep->delay;
        return 1;
    }
    if (!sweep->delay && (!logical_pose || sweep->duration <= 1)) {
        fade(sweep);
        return 1;
    }
    BfmForestSweepEdge current;
    if (!sample_edge(disc, logical_pose, model, &current)) return 0;
    if (sweep->delay) sweep->delay = 0;
    else --sweep->duration;
    return bfm_forest_sweep_append(sweep, &current);
}

static int project_point(const BfmForestCamera *camera, const int16_t point[3],
                         BfmForestRasterVertex *vertex, int *depth)
{
    int64_t view[3];
    for (unsigned axis = 0; axis < 3; ++axis) {
        int64_t dot = 0;
        for (unsigned i = 0; i < 3; ++i) dot += (int64_t)camera->rotation[axis][i] * point[i];
        view[axis] = camera->translation[axis] + floor_bits(dot, 12);
    }
    if (view[0] < INT16_MIN || view[0] > INT16_MAX || view[1] < INT16_MIN ||
        view[1] > INT16_MAX || view[2] <= 0 || view[2] > UINT16_MAX) return 0;
    int64_t scale = ((int64_t)camera->projection * 65536 + view[2] / 2) / view[2];
    if (scale > 131071) scale = 131071;
    int x = 160 + (int)floor_bits(view[0] * scale, 16);
    int y = 120 + (int)floor_bits(view[1] * scale, 16);
    if (x < -1024 || x > 1023 || y < -1024 || y > 1023) return 0;
    vertex->x = x;
    vertex->y = y;
    *depth = (int)view[2];
    return 1;
}

static unsigned shade(unsigned energy, unsigned age)
{
    unsigned fade_value = age * 16;
    unsigned blue = energy > fade_value ? energy - fade_value : 0;
    unsigned red = energy / 2 > fade_value ? energy / 2 - fade_value : 0;
    return red | red << 8 | blue << 16;
}

unsigned bfm_forest_sweep_project(const BfmForestSweep *sweep, const BfmForestCamera *camera,
                                  BfmForestDustCard out[BFM_FOREST_SWEEP_CARDS])
{
    if (!sweep || !camera || !camera->projection || !out || sweep->count < 2) return 0;
    unsigned count = 0;
    for (unsigned i = 1; i < sweep->count; ++i) {
        BfmForestDustCard card = {.material = {.page = 0x12a, .count = 4, .translucent = 1}};
        int depth = 0;
        unsigned corner;
        for (corner = 0; corner < 4; ++corner) {
            unsigned age = i - 1 + corner / 2;
            if (!project_point(camera, sweep->edges[age].point[corner % 2],
                               &card.vertices[corner], &depth)) break;
            card.vertices[corner].rgb = shade(sweep->energy, age);
        }
        if (corner != 4) continue;
        card.slot = i - 1;
        /* The older hilt owns this strip's depth bucket. */
        card.bucket = depth / 4 > 4095 ? 4095u : (unsigned)depth / 4;
        out[count++] = card;
    }
    return count;
}
