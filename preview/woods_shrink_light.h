/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_SHRINK_LIGHT_H
#define BFM_WOODS_SHRINK_LIGHT_H

#include "bedroom_figure.h"
#include "woods_shrink_shot.h"

enum { BFM_WOODS_SHRINK_MUZZLE_CARDS = 6 };

typedef struct {
    int32_t center[3];
    unsigned age;
    int scale;
    uint32_t color;
} BfmWoodsShrinkLight;

/* Start on the paid shot's emission. The muzzle keeps its birth position
 * and advances once per field, independently of a hit on its target. */
void bfm_woods_shrink_light_tick(BfmWoodsShrinkLight *light, const BfmWoodsShrinkShot *shot,
    const int32_t eye[3], unsigned field_tick, int emitted, unsigned equipped);
size_t bfm_woods_shrink_light_project(const BfmWoodsShrinkLight *light,
    const BfmForestCamera *camera, BfmForestDustCard *out, size_t capacity);

/* The release action supplies these three lights while drawing Musashi.
 * Other field actors use their usual lighting. */
void bfm_woods_shrink_player_lamps(unsigned field_tick, BfmBedroomLight lamps[3]);
#endif
