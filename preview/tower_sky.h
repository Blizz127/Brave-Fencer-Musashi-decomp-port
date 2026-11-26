/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_SKY_H
#define BFM_TOWER_SKY_H
#include "forest_terrain.h"
#include <stddef.h>
#include <stdint.h>

typedef struct BfmTowerSky BfmTowerSky;

/* Borrow the unpacked room and its uploaded video memory until free. */
BfmTowerSky *bfm_tower_sky_open(const uint8_t *scene, size_t scene_size,
                                const uint16_t (*tiles)[1024]);
BfmTowerSky *bfm_tower_summit_sky_open(const uint8_t *scene, size_t scene_size,
                                       const uint16_t (*tiles)[1024]);
/* A fixed short backdrop occupies the upper rows of the original scanout. */
BfmTowerSky *bfm_castle_drawbridge_sky_open(const uint8_t *scene, size_t scene_size,
                                           const uint16_t (*tiles)[1024]);
void bfm_tower_sky_free(BfmTowerSky *sky);

/* Draw before field geometry, with the same submitted camera. The room
 * transition fade applies to the final composite. */
int bfm_tower_sky_draw(const BfmTowerSky *sky, const BfmForestCamera *camera,
                       uint32_t pixels[240][320]);
#endif
