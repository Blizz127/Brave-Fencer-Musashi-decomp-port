/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_ROTATION_H
#define BFM_STEAM_KNIGHT_ROTATION_H
#include "steam_knight_flames.h"
enum { BFM_ROTATION_KEYS = 16, BFM_ROTATION_RAYS = 96 };
typedef struct {
    uint32_t body;
    int8_t anchors[2][2][3];
} BfmLuminaRotationKey;
typedef struct {
    BfmLuminaRotationKey keys[BFM_ROTATION_KEYS];
    unsigned count, held;
} BfmLuminaRotation;
int bfm_lumina_rotation_load(const MusashiDiscMedia *disc, BfmLuminaRotation *motion);
const BfmLuminaRotationKey *bfm_lumina_rotation_pose(const BfmLuminaRotation *motion,
    unsigned age, int16_t initial_heading, int16_t *heading);
size_t bfm_lumina_rotation_rays(const BfmLuminaRotation *motion, unsigned age,
    const int16_t position[3], int16_t initial_heading, BfmSteamFlameRay rays[BFM_ROTATION_RAYS]);
#endif
