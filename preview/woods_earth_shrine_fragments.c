/* SPDX-License-Identifier: MIT */
#include "woods_earth_shrine_fragments.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>

enum { FRAGMENTS = 16, STONE_MODELS = 3 };
static const uint32_t origin = UINT32_C(0x80128158);

typedef struct {
    int16_t offset[3], spin[3];
    uint16_t angles[3];
    int64_t xyz[3], velocity[3]; /* In 16.16 world units. */
    unsigned model, active;
} Fragment;

struct BfmWoodsEarthShrineFragments {
    Fragment pieces[FRAGMENTS];
    MusashiForestPlayerMesh *models[STONE_MODELS];
    int released;
};

static int floor_units(int64_t value, int unit)
{
    return (int)(value / unit - (value % unit < 0));
}

static void wave(uint16_t bits, int *sine, int *cosine)
{
    int angle = bits <= INT16_MAX ? bits : (int)bits - 65536;
    const int16_t zero[3] = {0};
    BfmForestTransform turn;
    bfm_forest_actor_root((unsigned)abs(angle), zero, &turn);
    *sine = angle < 0 ? -turn.rotation[0][2] : turn.rotation[0][2];
    *cosine = turn.rotation[0][0];
}

static int product(int a, int b)
{
    return floor_units((int64_t)a * b, 4096);
}

/* A rigid model spins about X, Y, then Z. The field quantizes its two
 * Y-tilted axes before the roll. This can move a lit edge by one unit. */
static BfmForestTransform stone_transform(const Fragment *piece)
{
    int sx, cx, sy, cy, sz, cz;
    wave(piece->angles[0], &sx, &cx);
    wave(piece->angles[1], &sy, &cy);
    wave(piece->angles[2], &sz, &cz);
    int tilt_y = product(sy, sx), tilt_z = product(sy, cx);
    BfmForestTransform root = {.rotation = {
        {product(cz, cy), product(cz, tilt_y) - product(sz, cx),
                          product(cz, tilt_z) + product(sz, sx)},
        {product(sz, cy), product(sz, tilt_y) + product(cz, cx),
                          product(sz, tilt_z) - product(cz, sx)},
        {-sy, product(cy, sx), product(cy, cx)}
    }};
    for (unsigned i = 0; i < 3; ++i) root.translation[i] = floor_units(piece->xyz[i], 65536);
    return root;
}

BfmWoodsEarthShrineFragments *bfm_woods_earth_shrine_fragments_open(
    const uint8_t *scene, size_t bytes)
{
    const size_t placements = UINT32_C(0x801e47c4) - origin;
    const size_t tables = UINT32_C(0x801e484c) - origin;
    if (!scene || bytes < tables + STONE_MODELS * 4) return NULL;
    BfmWoodsEarthShrineFragments *fragments = calloc(1, sizeof *fragments);
    if (!fragments) return NULL;
    for (unsigned i = 0; i < FRAGMENTS; ++i) {
        const uint8_t *row = scene + placements + i * 8;
        Fragment *piece = &fragments->pieces[i];
        for (unsigned k = 0; k < 3; ++k) piece->offset[k] = s16le(row + k * 2);
        piece->model = u16le(row + 6);
        if (!piece->offset[0] || piece->model >= STONE_MODELS) goto failed;
    }
    if (s16le(scene + placements + FRAGMENTS * 8)) goto failed;
    for (unsigned i = 0; i < STONE_MODELS; ++i) {
        unsigned count;
        fragments->models[i] = musashi_forest_player_mesh_from_table(
            scene, bytes, origin, u32le(scene + tables + i * 4), &count);
        if (!fragments->models[i] || count != 1) goto failed;
    }
    return fragments;
failed:
    bfm_woods_earth_shrine_fragments_free(fragments);
    return NULL;
}

void bfm_woods_earth_shrine_fragments_free(BfmWoodsEarthShrineFragments *fragments)
{
    if (!fragments) return;
    for (unsigned i = 0; i < STONE_MODELS; ++i)
        musashi_forest_player_mesh_free(fragments->models[i]);
    free(fragments);
}

int bfm_woods_earth_shrine_fragments_release(BfmWoodsEarthShrineFragments *fragments,
    const int16_t parent_xyz[3], unsigned parent_yaw, BfmGameplayRun *run)
{
    if (!fragments || !parent_xyz || !run || fragments->released) return 0;
    int sine, cosine;
    wave((uint16_t)parent_yaw, &sine, &cosine);
    for (unsigned i = 0; i < FRAGMENTS; ++i) {
        Fragment *piece = &fragments->pieces[i];
        const int16_t *offset = piece->offset;
        int rotated[3] = {
            floor_units((int64_t)cosine * offset[0] + (int64_t)sine * offset[2], 4096), offset[1],
            floor_units((int64_t)cosine * offset[2] - (int64_t)sine * offset[0], 4096)
        };
        for (unsigned k = 0; k < 3; ++k) {
            piece->xyz[k] = (int64_t)(parent_xyz[k] + rotated[k]) * 65536;
            piece->velocity[k] = (int64_t)floor_units(rotated[k], 8) * 65536;
            int spin = (int)(bfm_gameplay_random(run) & 2047u) - 1024;
            piece->spin[k] = (int16_t)floor_units(spin, 1 << piece->model);
        }
        piece->active = 1;
    }
    fragments->released = 1;
    return 1;
}

void bfm_woods_earth_shrine_fragments_step(BfmWoodsEarthShrineFragments *fragments, int parent_y)
{
    if (!fragments) return;
    for (unsigned i = 0; i < FRAGMENTS; ++i) {
        Fragment *piece = &fragments->pieces[i];
        if (!piece->active) continue;
        piece->velocity[1] += 98304;
        for (unsigned k = 0; k < 3; ++k) {
            piece->xyz[k] += piece->velocity[k];
            piece->angles[k] = (uint16_t)(piece->angles[k] + piece->spin[k]);
        }
        if (floor_units(piece->xyz[1], 65536) > parent_y) piece->active = 0;
    }
}

int bfm_woods_earth_shrine_fragments_draw(const BfmWoodsEarthShrineFragments *fragments,
    BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    if (!fragments || !figure || !view) return 0;
    static const BfmForestPose rigid = {
        .local = {{.rotation = {{4096,0,0},{0,4096,0},{0,0,4096}}}}
    };
    for (unsigned i = 0; i < FRAGMENTS; ++i) {
        const Fragment *piece = &fragments->pieces[i];
        if (!piece->active) continue;
        BfmForestTransform root = stone_transform(piece);
        BfmForestActor actor = {.pose = &rigid, .root = &root,
                                .mesh = fragments->models[piece->model], .visible_parts = 1};
        if (!bfm_bedroom_figure_add_actor(figure, view, &actor, NULL, NULL, 0)) return 0;
    }
    return 1;
}
