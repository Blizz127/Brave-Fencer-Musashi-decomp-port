/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_MUZZLE_ART_H
#define BFM_FOREST_MUZZLE_ART_H

#include "forest_bullet_art.h"

enum {
    BFM_FOREST_MUZZLE_FACES = 36, BFM_FOREST_MUZZLE_POINTS = 28,
    BFM_FOREST_MUZZLE_CARDS = BFM_FOREST_MUZZLE_FACES + BFM_GUNSHOT_FAN_CARDS
};

typedef struct {
    unsigned count;
    uint16_t vertex[4];
    uint32_t rgb[4];
} BfmForestMuzzleFace;

typedef struct {
    BfmForestMuzzleFace faces[BFM_FOREST_MUZZLE_FACES];
    int16_t points[BFM_FOREST_MUZZLE_POINTS][3];
    BfmForestBulletArt fan;
    BfmForestTransform model;
    unsigned light;
} BfmForestMuzzleArt;

/* Read the earned Gunshot resource's flash model and the authored fan. The
 * decoded data is owned by art and can outlive the original resource. */
int bfm_forest_muzzle_load(BfmForestMuzzleArt *art, const BfmForestAward *award);

/* One shared RNG draw selects the model's roll. The returned world position
 * is also where this emitter creates its separate camera-facing flare. */
int bfm_forest_muzzle_fire(BfmForestMuzzleArt *art, const BfmForestTransform *hero,
                           unsigned random, int16_t emitter[3]);
void bfm_forest_muzzle_tick(BfmForestMuzzleArt *art);
size_t bfm_forest_muzzle_project(const BfmForestMuzzleArt *art, const BfmForestCamera *camera,
                                 BfmForestDustCard *cards, size_t capacity);
#endif
