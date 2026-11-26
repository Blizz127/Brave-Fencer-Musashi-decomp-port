/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_BLADE_H
#define BFM_FOREST_BLADE_H
#include "forest_pose.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
    int8_t outline[2][9][2];
} BfmForestBlade;

typedef struct {
    BfmForestTransform world;
    unsigned tick;
} BfmForestBladeFrame;

/* Load the guard and blade outlines from the mounted scene in use. */
int bfm_forest_blade_load(BfmForestBlade *blade, const uint8_t *scene, size_t bytes);
int bfm_palace_blade_load(BfmForestBlade *blade, const uint8_t *scene, size_t bytes);

/* Orient the halo around the current weapon axis, toward the camera. */
int bfm_forest_blade_transform(const int8_t anchors[2][3], const BfmForestTransform *root,
                               const int32_t eye[3], BfmForestTransform *transform);
int bfm_forest_blade_quad(const BfmForestBlade *blade, unsigned face, unsigned tick,
                           int16_t xyz[4][3], uint32_t rgb[4]);
#endif
