/* SPDX-License-Identifier: MIT */
#include "forest_blade.h"
#include <math.h>
#include <string.h>

static int load_outlines(BfmForestBlade *blade, const uint8_t *scene, size_t bytes, size_t guard)
{
    /* Each nine-point signed pair list starts on a word boundary. */
    size_t blade_edge = guard + 20;
    if (!blade || !scene || bytes < blade_edge + sizeof(blade->outline[1])) return 0;
    memcpy(blade->outline[0], scene + guard, sizeof(blade->outline[0]));
    memcpy(blade->outline[1], scene + blade_edge, sizeof(blade->outline[1]));
    return 1;
}

int bfm_forest_blade_load(BfmForestBlade *blade, const uint8_t *scene, size_t bytes)
{
    return load_outlines(blade, scene, bytes, 0x61900);
}

int bfm_palace_blade_load(BfmForestBlade *blade, const uint8_t *scene, size_t bytes)
{
    return load_outlines(blade, scene, bytes, 0x60c50);
}

int bfm_library_blade_load(BfmForestBlade *blade, const uint8_t *scene, size_t bytes)
{
    return load_outlines(blade, scene, bytes, 0x5a574);
}

static int normalize(double v[3])
{
    double length = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if (length < 1e-9) return 0;
    for (unsigned i = 0; i < 3; ++i) v[i] /= length;
    return 1;
}

static void cross_product(const double a[3], const double b[3], double out[3])
{
    out[0] = a[1]*b[2] - a[2]*b[1];
    out[1] = a[2]*b[0] - a[0]*b[2];
    out[2] = a[0]*b[1] - a[1]*b[0];
}

int bfm_forest_blade_transform(const int8_t anchors[2][3], const BfmForestTransform *root,
                               const int32_t eye[3], BfmForestTransform *transform)
{
    if (!anchors || !root || !eye || !transform) return 0;
    double axis[3], toward[3], across[3], normal[3];
    BfmForestTransform result;
    for (unsigned i = 0; i < 3; ++i) {
        double center = 0, delta = 0;
        for (unsigned j = 0; j < 3; ++j) {
            double mid = floor((anchors[0][j] + anchors[1][j]) * 0.5);
            center += root->rotation[i][j] * mid;
            delta += root->rotation[i][j] * (anchors[1][j] - anchors[0][j]);
        }
        center = root->translation[i] + floor(center / 4096.0);
        if (center < INT16_MIN || center > INT16_MAX) return 0;
        result.translation[i] = (int32_t)center;
        toward[i] = (double)eye[i] - center;
        axis[i] = floor(delta / 4096.0);
    }
    /* The pose expresses the horizontal reach in whole model units before
     * determining pitch. Preserve the original bearing in the horizontal plane. */
    double horizontal = hypot(axis[0], axis[2]);
    if (horizontal > 0) {
        double scale = floor(horizontal) / horizontal;
        axis[0] *= scale;
        axis[2] *= scale;
    }
    if (!normalize(toward) || !normalize(axis)) return 0;
    for (unsigned i = 0; i < 3; ++i) {
        result.translation[i] += (int32_t)floor(16.0 * toward[i]);
        toward[i] = (double)eye[i] - result.translation[i];
    }
    cross_product(toward, axis, across);
    if (!normalize(across)) return 0;
    cross_product(axis, across, normal);
    for (unsigned i = 0; i < 3; ++i) {
        result.rotation[i][0] = (int16_t)lround(across[i] * 4096.0);
        result.rotation[i][1] = (int16_t)lround(normal[i] * 4096.0);
        result.rotation[i][2] = (int16_t)lround(axis[i] * 4096.0);
    }
    *transform = result;
    return 1;
}

int bfm_forest_blade_quad(const BfmForestBlade *blade, unsigned face, unsigned tick,
                           int16_t xyz[4][3], uint32_t rgb[4])
{
    if (!blade || !xyz || !rgb || face >= 8) return 0;
    unsigned band = face / 4, start = 2 * (face % 4);
    unsigned blue = ((tick & 1u) ? 72u : 88u) * (band + 1u);
    uint32_t light = (blue << 16) | ((blue / 4) << 8) | (blue / 4);
    for (unsigned i = 0; i < 4; ++i) {
        unsigned corner = start + (i >= 2 ? i - 1 : 0);
        int x = i == 1 ? 0 : blade->outline[band][corner][0];
        int y = i == 1 ? (band ? -18 : 0) : blade->outline[band][corner][1];
        xyz[i][0] = (int16_t)x;
        xyz[i][1] = band ? 0 : (int16_t)y;
        xyz[i][2] = band ? (int16_t)y : 0;
        rgb[i] = (i == 1 || (face == 6 && i == 3) || (face == 7 && i == 0)) ? light : 0;
    }
    return 1;
}
