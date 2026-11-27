/* SPDX-License-Identifier: MIT */
#include "cable_sparks.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    unsigned birth;
    int origin[3];
    int16_t velocity[2][3];
} SparkBurst;

struct BfmCableSparks {
    int axes[4][3][2];
    BfmForestTransform heading;
    unsigned movement, last_birth;
    SparkBurst bursts[2];
};

static int floor_q12(int64_t value)
{
    return (int)(value / 4096 - (value % 4096 < 0));
}

static unsigned angle(const uint8_t *p)
{
    return (p[0] | (unsigned)p[1] << 8) & 4095u;
}

static void fork_axes(const uint8_t angles[4], const BfmForestTransform *basis, int axes[3][2])
{
    const int16_t zero[3] = {0};
    BfmForestTransform pitch, roll;
    bfm_forest_actor_root(angle(angles), zero, &pitch);
    bfm_forest_actor_root(angle(angles + 2), zero, &roll);
    int sx = pitch.rotation[0][2], cx = pitch.rotation[0][0];
    int sz = roll.rotation[0][2], cz = roll.rotation[0][0];
    /* The fork lies in its local XZ plane. Apply its pitch and roll,
     * then transform those two axes through the emitting surface. */
    int xz = floor_q12(sz * sx), yz = -floor_q12(cz * sx);
    for (unsigned k = 0; k < 3; ++k) {
        const int16_t *row = basis->rotation[k];
        axes[k][0] = floor_q12((int64_t)row[0] * cz + (int64_t)row[1] * sz);
        axes[k][1] = floor_q12((int64_t)row[0] * xz + (int64_t)row[1] * yz + (int64_t)row[2] * cx);
    }
}

BfmCableSparks *bfm_cable_sparks_open_yaw(const uint8_t *orientations, size_t size, unsigned yaw)
{
    if (!orientations || size < 16) return NULL;
    BfmCableSparks *sparks = calloc(1, sizeof *sparks);
    if (!sparks) return NULL;
    const int16_t zero[3] = {0};
    bfm_forest_actor_root(yaw, zero, &sparks->heading);
    for (unsigned i = 0; i < 4; ++i)
        fork_axes(orientations + 4*i, &sparks->heading, sparks->axes[i]);
    return sparks;
}

BfmCableSparks *bfm_cable_sparks_open(const uint8_t *orientations, size_t size)
{
    return bfm_cable_sparks_open_yaw(orientations,size,2048);
}

void bfm_cable_sparks_free(BfmCableSparks *sparks)
{
    free(sparks);
}

void bfm_field_spark_velocity(const BfmForestTransform *basis, uint16_t random, int16_t velocity[3])
{
    if (!basis || !velocity) return;
    const int16_t zero[3] = {0};
    BfmForestTransform direction;
    unsigned pitch = 8u * (random & 127u);
    unsigned yaw = 3072u + 8u * ((random >> 8) & 127u);
    bfm_forest_actor_pitch_yaw(pitch, yaw, zero, &direction);
    for (unsigned k = 0; k < 3; ++k) {
        int64_t sum = 0;
        for (unsigned j = 0; j < 3; ++j)
            sum += (int64_t)basis->rotation[k][j] * direction.rotation[j][2];
        velocity[k] = (int16_t)floor_q12(36 * floor_q12(sum));
    }
}

void bfm_cable_sparks_advance(BfmCableSparks *sparks, unsigned movement, int emit,
                              const int16_t position[3], BfmGameplayRun *run)
{
    if (!sparks || !position || !run || movement <= sparks->movement) return;
    sparks->movement = movement;
    if (!emit || (sparks->last_birth && movement - sparks->last_birth < 4)) return;
    sparks->last_birth = movement;
    SparkBurst *burst = &sparks->bursts[(movement / 4) % 2];
    burst->birth = movement;
    burst->origin[0] = position[0] + floor_q12(-8 * sparks->heading.rotation[0][2]);
    burst->origin[1] = position[1] - 68;
    burst->origin[2] = position[2] + floor_q12(-8 * sparks->heading.rotation[2][2]);
    for (unsigned i = 0; i < 2; ++i)
        bfm_field_spark_velocity(&sparks->heading, bfm_gameplay_random(run), burst->velocity[i]);
}

void bfm_cable_sparks_step(BfmCableSparks *sparks, unsigned movement,
                           const int16_t position[3], BfmGameplayRun *run)
{
    bfm_cable_sparks_advance(sparks,movement,movement % 4 == 0,position,run);
}

static int project_point(const BfmBedroomView *view, const int world[3],
                         BfmForestRasterVertex *vertex, int *depth)
{
    int16_t xyz[3];
    for (unsigned k = 0; k < 3; ++k) {
        if (world[k] < INT16_MIN || world[k] > INT16_MAX) return 0;
        xyz[k] = (int16_t)world[k];
    }
    int screen[3];
    if (!bfm_bedroom_project(view, xyz, screen) || screen[2] <= view->focal/2) return 0;
    vertex->x = screen[0];
    vertex->y = screen[1];
    *depth = screen[2];
    return 1;
}

static int project_ray(const int origin[3], const int axes[3][2], unsigned age,
                       const BfmBedroomView *view, BfmCableSparkFace *out)
{
    static const int points[3][2] = {{0,2}, {-3,36}, {3,40}};
    BfmCableSparkFace face = {.count = 3};
    int depth;
    for (unsigned i = 0; i < 3; ++i) {
        int world[3];
        for (unsigned k = 0; k < 3; ++k) {
            int offset = axes[k][0]*points[i][0] + axes[k][1]*points[i][1];
            world[k] = origin[k] + floor_q12((int64_t)offset * (2+age) / 2);
        }
        if (!project_point(view, world, &face.vertices[i], &depth)) return 0;
        face.vertices[i].rgb = i ? 0x40c0ffu : 0x104040u;
    }
    face.depth = depth / 4;
    *out = face;
    return 1;
}

static int project_trail(const int origin[3], const int16_t velocity[3], unsigned age,
                         const BfmBedroomView *view, BfmCableSparkFace *out)
{
    int from[3], to[3];
    for (unsigned k = 0; k < 3; ++k) {
        from[k] = to[k] = origin[k];
        unsigned fraction = 0;
        for (unsigned i = 1; i < age; ++i) {
            from[k] = to[k];
            /* The previous integral coordinate occupies the fractional half
             * of the particle's velocity during its next Q16 integration. */
            fraction += (uint16_t)from[k];
            to[k] += velocity[k] + (int)(fraction >> 16);
            fraction &= 65535u;
        }
    }
    BfmCableSparkFace face = {.count = 2};
    int depth;
    if (!project_point(view, from, &face.vertices[0], &depth) ||
        !project_point(view, to, &face.vertices[1], &depth)) return 0;
    face.vertices[0].rgb = 0x000008;
    face.vertices[1].rgb = 0x8080c0;
    face.depth = depth / 4;
    *out = face;
    return 1;
}

static void depth_card(const BfmCableSparkFace *face, BfmForestDustCard *card)
{
    *card = (BfmForestDustCard){
        .bucket = (unsigned)face->depth,
        .material = {.count = face->count, .page = 1u << 5, .translucent = 1}
    };
    memcpy(card->vertices, face->vertices, sizeof face->vertices);
}

unsigned bfm_field_spark_fork(const BfmForestTransform *basis, const uint8_t *orientations,
                              size_t size, const int16_t origin[3], unsigned age,
                              const BfmBedroomView *view, BfmForestDustCard out[4])
{
    if (!basis || !orientations || size < 16 || !origin || age >= 3 ||
        !view || view->focal <= 0 || !out) return 0;
    const int xyz[3] = {origin[0], origin[1], origin[2]};
    unsigned count = 0;
    for (unsigned i = 0; i < 4; ++i) {
        int axes[3][2];
        BfmCableSparkFace face;
        fork_axes(orientations + 4*i, basis, axes);
        if (project_ray(xyz, axes, age, view, &face)) depth_card(&face, &out[count++]);
    }
    return count;
}

int bfm_field_spark_trail(const int16_t origin[3], const int16_t velocity[3], unsigned age,
                          const BfmBedroomView *view, BfmForestDustCard *out)
{
    if (!origin || !velocity || !age || age > 5 || !view || view->focal <= 0 || !out) return 0;
    const int xyz[3] = {origin[0], origin[1], origin[2]};
    BfmCableSparkFace face;
    if (!project_trail(xyz, velocity, age, view, &face)) return 0;
    depth_card(&face, out);
    return 1;
}

unsigned bfm_cable_sparks_project(BfmCableSparks *sparks, const BfmBedroomView *view,
                                  BfmCableSparkFace out[8])
{
    if (!sparks || !view || !out || view->focal <= 0) return 0;
    unsigned count = 0;
    for (unsigned b = 0; b < 2; ++b) {
        const SparkBurst *burst = &sparks->bursts[b];
        if (!burst->birth) continue;
        unsigned age = sparks->movement - burst->birth;
        if (age < 3) {
            for (unsigned i = 0; i < 4; ++i)
                count += project_ray(burst->origin, sparks->axes[i], age, view, &out[count]);
        }
        if (age > 0 && age <= 5) {
            for (unsigned i = 0; i < 2; ++i)
                count += project_trail(burst->origin, burst->velocity[i], age, view, &out[count]);
        }
    }
    return count;
}

void bfm_cable_sparks_draw(const BfmForestRaster *surface, const BfmCableSparkFace *face)
{
    if (!surface || !surface->pixels || !face) return;
    if (face->count == 2) {
        bfm_forest_raster_additive_line(surface, face->vertices);
    } else if (face->count == 3) {
        const BfmForestRasterMaterial material = {.page = 0x12a, .count = 3, .translucent = 1};
        bfm_forest_raster_face(surface, &material, face->vertices);
    }
}
