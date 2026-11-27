/* SPDX-License-Identifier: MIT */
#include "woods_charge_core.h"
#include "forest_bullet_art.h"
#include "forest_shore_motion.h"

#include <math.h>
#include <string.h>

int bfm_woods_charge_core_load(BfmWoodsChargeCore *art, const uint8_t *scene, size_t bytes)
{
    const size_t rim = UINT32_C(0x8018a8f8) - UINT32_C(0x80128158);
    const size_t blade = UINT32_C(0x8018a90c) - UINT32_C(0x80128158);
    if (!art || !scene || bytes < blade + 28 || memcmp(scene + rim, scene + rim + 16, 2))
        return 0;
    *art = (BfmWoodsChargeCore){.rim = scene + rim, .blade = scene + blade};
    return 1;
}

static int q12(int64_t value)
{
    return (int)(value / 4096 - (value % 4096 < 0));
}

static int angle(int horizontal, int forward)
{
    int a = (int)bfm_forest_shore_bearing(horizontal, forward);
    return a > 2048 ? a - 4096 : a;
}

static void wave(int a, int *sine, int *cosine)
{
    BfmForestTransform turn;
    const int16_t origin[3] = {0};
    bfm_forest_actor_root((unsigned)(a < 0 ? -a : a), origin, &turn);
    *sine = a < 0 ? -turn.rotation[0][2] : turn.rotation[0][2];
    *cosine = turn.rotation[0][0];
}

/* The direction calculation keeps one base-four mantissa byte. Evaluating
 * its square root at that precision preserves the near-horizontal angles. */
static int horizontal_length(int x, int z)
{
    unsigned squared = (unsigned)(x * x + z * z);
    if (!squared) return 0;
    unsigned numerator = 1, denominator = 512;
    while (squared > 255) {
        squared /= 4;
        numerator *= 2;
    }
    while (squared < 64) {
        squared *= 4;
        denominator *= 2;
    }
    unsigned mantissa = (unsigned)(sqrt((double)squared) * 512);
    return (int)(mantissa * numerator / denominator);
}

static void orient(BfmForestTransform *mesh, const BfmForestTransform *model,
                   const int8_t anchors[2][3], const int32_t eye[3])
{
    int edge[3];
    for (unsigned axis = 0; axis < 3; ++axis) {
        int sum = 0;
        for (unsigned k = 0; k < 3; ++k)
            sum += model->rotation[axis][k] * (anchors[1][k] - anchors[0][k]);
        edge[axis] = q12(sum);
    }
    /* Point local Z along the blade. Then turn the broad side toward the eye
     * using the eye's two coordinates in the plane perpendicular to Z. */
    int sx, cx, sy, cy;
    wave(-angle(edge[1], horizontal_length(edge[0], edge[2])), &sx, &cx);
    wave(angle(edge[0], edge[2]), &sy, &cy);
    int xaxis[3] = {cy, 0, -sy};
    int yaxis[3] = {q12(sy * sx), cx, q12(cy * sx)};
    int64_t across = 0, up = 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        int64_t distance = (int64_t)eye[axis] - mesh->translation[axis];
        across += distance * xaxis[axis];
        up += distance * yaxis[axis];
    }
    int sr, cr;
    wave(-angle(q12(across), q12(up)), &sr, &cr);
    mesh->rotation[0][0] = (int16_t)(q12(cy * cr) + q12(yaxis[0] * sr));
    mesh->rotation[0][1] = (int16_t)(q12(yaxis[0] * cr) - q12(cy * sr));
    mesh->rotation[0][2] = (int16_t)q12(sy * cx);
    mesh->rotation[1][0] = (int16_t)q12(cx * sr);
    mesh->rotation[1][1] = (int16_t)q12(cx * cr);
    mesh->rotation[1][2] = (int16_t)-sx;
    mesh->rotation[2][0] = (int16_t)(q12(yaxis[2] * sr) - q12(sy * cr));
    mesh->rotation[2][1] = (int16_t)(q12(yaxis[2] * cr) + q12(sy * sr));
    mesh->rotation[2][2] = (int16_t)q12(cy * cx);
}

static void rim_point(int16_t point[3], const uint8_t *pair)
{
    point[0] = (int16_t)(pair[0] < 128 ? pair[0] : (int)pair[0] - 256);
    point[2] = (int16_t)(pair[1] < 128 ? pair[1] : (int)pair[1] - 256);
}

size_t bfm_woods_charge_core_project(const BfmWoodsChargeCore *art, const int32_t tip[3],
    const BfmForestTransform *model, const int8_t anchors[2][3], unsigned room_tick,
    unsigned gauge, const int32_t eye[3], const BfmForestCamera *camera,
    BfmForestDustCard *cards, size_t capacity)
{
    if (!art || !art->rim || !art->blade || !tip || !model || !anchors || !eye || !camera ||
        !cards || !capacity || gauge < 24 || gauge > 128) return 0;
    BfmForestTransform mesh;
    bfm_forest_glow_facing(eye, tip, 8, &mesh);
    orient(&mesh, model, anchors, eye);
    BfmForestCamera local;
    if (!bfm_forest_effect_camera(camera, &mesh, &local)) return 0;

    unsigned flash = room_tick & 1 ? 32 : (room_tick & 2 ? 64 : 0);
    unsigned red = gauge - 24 + flash;
    uint32_t rgb = red | ((red / 8) << 8) | ((red / 8) << 16);
    size_t count = bfm_forest_glow_fan_project(art->rim, 4, &mesh, 0, rgb, camera, cards, capacity);
    /* Two fans along the blade have separate bright hubs. Two closing quads
     * connect those hubs; their two outside corners stay dark. */
    for (unsigned strip = 0; strip < 6 && count < capacity; ++strip) {
        int16_t corners[4][3] = {{0}};
        BfmForestDustCard card = {.material = {.count = 4, .page = 32, .translucent = 1}};
        card.vertices[1].rgb = rgb;
        if (strip < 4) {
            const uint8_t *rim = art->blade + 4 * strip + (strip < 2 ? 0 : 2);
            corners[1][2] = strip < 2 ? -32 : 32;
            rim_point(corners[0], rim);
            rim_point(corners[2], rim + 2);
            rim_point(corners[3], rim + 4);
        } else {
            const uint8_t *rim = art->blade + 20 + 4 * (strip - 4);
            corners[0][2] = -32;
            corners[1][2] = 32;
            card.vertices[0].rgb = rgb;
            rim_point(corners[2], rim);
            rim_point(corners[3], rim + 2);
        }
        if (bfm_forest_project_effect_quad(&local, corners, &card)) cards[count++] = card;
    }
    return count;
}
