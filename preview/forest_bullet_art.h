/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_BULLET_ART_H
#define BFM_FOREST_BULLET_ART_H

#include "forest_award.h"
#include "forest_dust.h"

enum { BFM_GUNSHOT_FAN_CARDS = 4, BFM_GUNSHOT_PERIMETER_BYTES = 18 };

typedef struct {
    uint8_t perimeter[BFM_GUNSHOT_PERIMETER_BYTES];
    BfmForestTransform flash;
    unsigned brightness;
    int loaded;
} BfmForestBulletArt;

int bfm_forest_bullet_art_load(BfmForestBulletArt *art, const BfmForestAward *award);

/* Fire the pale muzzle flash at the actual emitter position. Its plane faces
 * the field camera at birth and stays there for its brief lifetime. */
void bfm_forest_bullet_art_fire(BfmForestBulletArt *art, const int16_t muzzle[3],
                                const int32_t eye[3]);
void bfm_forest_bullet_art_tick(BfmForestBulletArt *art);
size_t bfm_forest_bullet_art_project(const BfmForestBulletArt *art,
                   const BfmForestCamera *camera, BfmForestDustCard *cards, size_t capacity);

/* Construct a spherical billboard toward the field eye. An optional shift
 * moves its center toward the eye by that fraction of the Q12 direction.
 * The signed pitch is also used by the attached blade's larger halo. */
int bfm_forest_glow_facing(const int32_t eye[3], const int32_t center[3],
                           unsigned shift, BfmForestTransform *out);

/* Two triangles per quad around a shared center. The mounted perimeter
 * holds 2*quads+1 signed (x,y) or (x,z) pairs, including the closing point. */
size_t bfm_forest_glow_fan_project(const uint8_t *perimeter, unsigned quads,
                    const BfmForestTransform *root, int plane_xz, uint32_t rgb,
                    const BfmForestCamera *camera, BfmForestDustCard *cards, size_t capacity);

/* Both muzzle effects use the mounted signed perimeter and the same fan.
 * The white emission lies in local XZ; the yellow flare lies in local XY. */
size_t bfm_forest_bullet_fan_project(const uint8_t perimeter[BFM_GUNSHOT_PERIMETER_BYTES],
                    const BfmForestTransform *root, int plane_xz, uint32_t rgb,
                    const BfmForestCamera *camera, BfmForestDustCard *cards, size_t capacity);
#endif
