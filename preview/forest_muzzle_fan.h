/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_MUZZLE_FAN_H
#define BFM_FOREST_MUZZLE_FAN_H

#include "forest_field.h"

enum { BFM_FOREST_MUZZLE_FAN_CARDS = 8 };

typedef struct {
    int32_t center[3];
    int16_t axes[3][2];
} BfmForestMuzzlePlane;

typedef struct {
    BfmForestMuzzlePlane planes[2];
    int8_t rim[9][2];
    unsigned plane_count;
} BfmForestMuzzleFan;

/* Both flash planes are fixed when the rifle fires. The floor plane requires
 * a reachable surface. Their silhouettes follow the current camera. */
int bfm_forest_muzzle_fan_start(BfmForestMuzzleFan *fan, const BfmForestField *field,
                                const int16_t muzzle[3], const int32_t birth_eye[3]);
unsigned bfm_forest_muzzle_fan_project(const BfmForestMuzzleFan *fan,
                                      const BfmForestCamera *camera, unsigned brightness,
                                      BfmForestDustCard out[BFM_FOREST_MUZZLE_FAN_CARDS]);
#endif
