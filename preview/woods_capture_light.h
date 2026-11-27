/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_CAPTURE_LIGHT_H
#define BFM_WOODS_CAPTURE_LIGHT_H

#include "forest_tether_trails.h"

enum { BFM_WOODS_CAPTURE_MOTES = 20,
       BFM_WOODS_CAPTURE_LIGHT_CARDS = (BFM_WOODS_CAPTURE_MOTES + 1) * 6 };

typedef struct {
    int64_t position[3]; /* 16.16 world coordinates. */
    int32_t velocity[3];
    int scale, remaining;
} BfmWoodsCaptureMote;

typedef struct {
    BfmWoodsCaptureMote motes[BFM_WOODS_CAPTURE_MOTES];
    int32_t anchor[3], center[3];
    int held, burst_ticks, show, scale_x, scale_y;
} BfmWoodsCaptureLight;

/* Zero-initialize before entering the room. Supply the victim's root while a
 * genuine tether holds it. The final full-meter update supplies earned once.
 * The burst and its drifting motes continue after the caster disappears. */
void bfm_woods_capture_light_tick(BfmWoodsCaptureLight *light, const int16_t target_root[3],
    int earned, const int32_t eye[3], BfmTetherRandom random, void *rng_context);

/* A camera-parallel field light has twelve dark perimeter vertices and one
 * colored center. The display scale applies after rounding the base radius. */
size_t bfm_woods_radial_light_project(const BfmForestCamera *camera, const int32_t center[3],
    int radius, int scale, uint32_t color, BfmForestDustCard *out, size_t capacity);
size_t bfm_woods_radial_light_project_axes(const BfmForestCamera *camera, const int32_t center[3],
    int radius, int scale_x, int scale_y, uint32_t color, BfmForestDustCard *out, size_t capacity);

size_t bfm_woods_capture_light_project(const BfmWoodsCaptureLight *light,
    const BfmForestCamera *camera, const int32_t eye[3], BfmForestDustCard *out, size_t capacity);

#endif
