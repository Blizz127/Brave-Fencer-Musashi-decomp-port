/* SPDX-License-Identifier: MIT */
#include "forest_sparks.h"
#include <math.h>
#include <string.h>

static int floor_shift(int64_t value, unsigned bits)
{
    int64_t divisor = INT64_C(1) << bits;
    return (int)(value / divisor - (value % divisor < 0));
}

void bfm_forest_sparks_start(BfmForestSparks *sparks, const int32_t origin[3],
                             const BfmForestTransform *face)
{
    memcpy(sparks->origin, origin, sizeof sparks->origin);
    memcpy(sparks->face, face->rotation, sizeof sparks->face);
    sparks->remaining = BFM_FOREST_STRIKE_SPARKS;
}

static void emit(BfmForestSparks *sparks, BfmGameplayRun *run, const int32_t eye[3])
{
    unsigned index = BFM_FOREST_STRIKE_SPARKS - sparks->remaining--;
    BfmForestSpark *spark = &sparks->sparks[index];
    uint16_t roll = bfm_gameplay_random(run);
    const int16_t zero[3] = {0};
    BfmForestTransform scatter;
    bfm_forest_actor_pitch_yaw((roll & 127) * 8, 3072 + ((roll >> 8) & 127) * 8, zero, &scatter);
    double dx = (double)eye[0] - sparks->origin[0], dy = (double)eye[1] - sparks->origin[1];
    double dz = (double)eye[2] - sparks->origin[2];
    double units = 2048.0 / acos(-1.0);
    int yaw = (int)lround(atan2(dx, dz) * units);
    int pitch = -(int)lround(atan2(dy, hypot(dx, dz)) * units);
    BfmForestTransform facing;
    bfm_forest_actor_pitch_yaw((unsigned)pitch, (unsigned)yaw, zero, &facing);
    *spark = (BfmForestSpark){.alive = 1};
    for (unsigned i = 0; i < 3; ++i) {
        int32_t direction = 0;
        for (unsigned k = 0; k < 3; ++k)
            direction += sparks->face[i][k] * scatter.rotation[k][2];
        spark->position[i] = sparks->origin[i];
        spark->velocity[i] = (int16_t)floor_shift(10 * floor_shift(direction, 12), 12);
        spark->billboard[i][0] = facing.rotation[i][0];
        spark->billboard[i][1] = facing.rotation[i][1];
    }
}

void bfm_forest_sparks_tick(BfmForestSparks *sparks, BfmGameplayRun *run, const int32_t eye[3])
{
    for (unsigned i = 0; i < BFM_FOREST_STRIKE_SPARKS; ++i) {
        BfmForestSpark *spark = &sparks->sparks[i];
        if (!spark->alive) continue;
        if (spark->age == 10) {
            spark->alive = 0;
            continue;
        }
        memcpy(spark->submitted, spark->position, sizeof spark->submitted);
        for (unsigned k = 0; k < 3; ++k) spark->position[k] += spark->velocity[k];
        ++spark->velocity[1];
        ++spark->age;
    }
    if (sparks->remaining && run) emit(sparks, run, eye);
}

static int project_shape(const BfmForestCamera *camera, const int32_t center[3],
                         const int axes[3][2], int diamond, unsigned brightness,
                         unsigned slot, BfmForestDustCard *out)
{
    int depth = 0;
    for (unsigned i = 0; i < 4; ++i) {
        int horizontal = (i & 2) ? 1 : -1, vertical = (i & 1) ? 1 : -1;
        int x = diamond ? (horizontal + vertical) * 7 / 2 : horizontal * 5;
        int y = diamond ? (vertical - horizontal) * 7 / 2 : vertical * 5;
        int view[3];
        for (unsigned k = 0; k < 3; ++k)
            view[k] = center[k] + floor_shift(axes[k][0] * x + axes[k][1] * y, 12);
        if (view[2] <= 0 || view[2] > 65535) return 0;
        depth = view[2] / 4;
        unsigned q = (unsigned)(((int64_t)camera->projection * 65536 + view[2] / 2) / view[2]);
        if (q > 131071) q = 131071;
        out->vertices[i] = (BfmForestRasterVertex){
            .x = 160 + floor_shift((int64_t)view[0] * q, 16),
            .y = 120 + floor_shift((int64_t)view[1] * q, 16), .rgb = brightness * UINT32_C(0x010101)
        };
    }
    out->bucket = (unsigned)(depth > 4095 ? 4095 : depth);
    out->slot = slot;
    out->material = (BfmForestRasterMaterial){.page = 298, .count = 4, .translucent = 1};
    return 1;
}

unsigned bfm_forest_sparks_project(const BfmForestSparks *sparks, const BfmForestCamera *camera,
                                   BfmForestDustCard out[BFM_FOREST_SPARK_CARDS])
{
    unsigned count = 0;
    for (unsigned i = 0; i < BFM_FOREST_STRIKE_SPARKS; ++i) {
        const BfmForestSpark *spark = &sparks->sparks[i];
        if (!spark->alive || !spark->age) continue;
        unsigned elapsed = spark->age - 1;
        int scale = ((i & 1) ? 1228 : 2048) + 128 * elapsed;
        int32_t center[3];
        int axes[3][2];
        for (unsigned k = 0; k < 3; ++k) {
            int64_t center_dot = 0;
            int32_t x_dot = 0, y_dot = 0;
            for (unsigned t = 0; t < 3; ++t) {
                int weight = camera->rotation[k][t];
                center_dot += (int64_t)weight * spark->submitted[t];
                x_dot += weight * floor_shift(spark->billboard[t][0] * scale, 12);
                y_dot += weight * floor_shift(spark->billboard[t][1] * scale, 12);
            }
            center[k] = camera->translation[k] + floor_shift(center_dot, 12);
            axes[k][0] = floor_shift(x_dot, 12);
            axes[k][1] = floor_shift(y_dot, 12);
        }
        /* The depth table walks equal-depth quads in reverse insertion order. */
        for (unsigned shape = 0; shape < 2; ++shape)
            count += project_shape(camera, center, axes, (int)shape, 192 - 16 * elapsed,
                                   2 * i + shape, &out[count]);
    }
    return count;
}
