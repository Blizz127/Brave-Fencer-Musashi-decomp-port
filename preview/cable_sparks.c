/* SPDX-License-Identifier: MIT */
#include "cable_sparks.h"
#include "forest_pose.h"

#include <limits.h>
#include <stdlib.h>

typedef struct {
    unsigned birth;
    int origin[3], velocity[2][3];
} SparkBurst;

struct BfmCableSparks {
    int axes[4][3][2];
    int sine, cosine;
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

BfmCableSparks *bfm_cable_sparks_open_yaw(const uint8_t *orientations, size_t size, unsigned yaw)
{
    if (!orientations || size < 16) return NULL;
    BfmCableSparks *sparks = calloc(1, sizeof *sparks);
    if (!sparks) return NULL;
    const int16_t zero[3] = {0};
    BfmForestTransform heading;
    bfm_forest_actor_root(yaw, zero, &heading);
    int sine = sparks->sine = heading.rotation[0][2];
    int cosine = sparks->cosine = heading.rotation[0][0];
    for (unsigned i = 0; i < 4; ++i) {
        BfmForestTransform pitch, roll;
        bfm_forest_actor_root(angle(orientations + 4*i), zero, &pitch);
        bfm_forest_actor_root(angle(orientations + 4*i + 2), zero, &roll);
        int sx = pitch.rotation[0][2], cx = pitch.rotation[0][0];
        int sz = roll.rotation[0][2], cz = roll.rotation[0][0];
        /* Compose the fan's pitch and roll with the rider's heading. Only
         * its local X and Z columns contribute to the projected triangles. */
        int xz = floor_q12(sz * sx);
        int (*axis)[2] = sparks->axes[i];
        axis[0][0] = floor_q12(cosine * cz);
        axis[1][0] = sz;
        axis[2][0] = floor_q12(-sine * cz);
        axis[0][1] = floor_q12(cosine * xz + sine * cx);
        axis[1][1] = -floor_q12(cz * sx);
        axis[2][1] = floor_q12(-sine * xz + cosine * cx);
    }
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

void bfm_cable_sparks_advance(BfmCableSparks *sparks, unsigned movement, int emit,
                              const int16_t position[3], BfmGameplayRun *run)
{
    if (!sparks || !position || !run || movement <= sparks->movement) return;
    sparks->movement = movement;
    if (!emit || (sparks->last_birth && movement - sparks->last_birth < 4)) return;
    sparks->last_birth = movement;
    SparkBurst *burst = &sparks->bursts[(movement / 4) % 2];
    burst->birth = movement;
    burst->origin[0] = position[0] + floor_q12(-8 * sparks->sine);
    burst->origin[1] = position[1] - 68;
    burst->origin[2] = position[2] + floor_q12(-8 * sparks->cosine);
    for (unsigned i = 0; i < 2; ++i) {
        uint16_t random = bfm_gameplay_random(run);
        BfmForestTransform direction;
        const int16_t zero[3] = {0};
        unsigned pitch = 8u * (random & 127u);
        unsigned yaw = 3072u + 8u * ((random >> 8) & 127u);
        bfm_forest_actor_pitch_yaw(pitch, yaw, zero, &direction);
        int x = direction.rotation[0][2], z = direction.rotation[2][2];
        int dx = floor_q12(sparks->cosine*x + sparks->sine*z);
        int dz = floor_q12(-sparks->sine*x + sparks->cosine*z);
        burst->velocity[i][0] = floor_q12(36 * dx);
        burst->velocity[i][1] = floor_q12(36 * direction.rotation[1][2]);
        burst->velocity[i][2] = floor_q12(36 * dz);
    }
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

static int project_ray(const SparkBurst *burst, int axes[3][2], unsigned age,
                       const BfmBedroomView *view, BfmCableSparkFace *out)
{
    static const int points[3][2] = {{0,2}, {-3,36}, {3,40}};
    BfmCableSparkFace face = {.count = 3};
    int depth;
    for (unsigned i = 0; i < 3; ++i) {
        int world[3];
        for (unsigned k = 0; k < 3; ++k) {
            int offset = axes[k][0]*points[i][0] + axes[k][1]*points[i][1];
            world[k] = burst->origin[k] + floor_q12((int64_t)offset * (2+age) / 2);
        }
        if (!project_point(view, world, &face.vertices[i], &depth)) return 0;
        face.vertices[i].rgb = i ? 0x40c0ffu : 0x104040u;
    }
    face.depth = depth / 4;
    *out = face;
    return 1;
}

static int project_trail(const SparkBurst *burst, unsigned index, unsigned age,
                         const BfmBedroomView *view, BfmCableSparkFace *out)
{
    int from[3], to[3];
    for (unsigned k = 0; k < 3; ++k) {
        from[k] = to[k] = burst->origin[k];
        unsigned fraction = 0;
        for (unsigned i = 1; i < age; ++i) {
            from[k] = to[k];
            /* The previous integral coordinate occupies the fractional half
             * of the particle's velocity during its next Q16 integration. */
            fraction += (uint16_t)from[k];
            to[k] += burst->velocity[index][k] + (int)(fraction >> 16);
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
                count += project_ray(burst, sparks->axes[i], age, view, &out[count]);
        }
        if (age > 0 && age <= 5) {
            for (unsigned i = 0; i < 2; ++i)
                count += project_trail(burst, i, age, view, &out[count]);
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
