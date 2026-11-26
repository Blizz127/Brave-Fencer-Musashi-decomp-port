/* SPDX-License-Identifier: MIT */
#ifndef BFM_HERO_SMOKE_MATERIAL_H
#define BFM_HERO_SMOKE_MATERIAL_H

#include <stddef.h>
#include <stdint.h>

enum { BFM_HERO_SMOKE_TRACKS = 3, BFM_HERO_SMOKE_CELLS = 10, BFM_HERO_SMOKE_ORIGINS = 12 };

typedef struct {
    uint16_t page, palette;
    uint8_t uv[4][2];
    uint8_t fields;
} BfmHeroSmokeFrame;

typedef struct {
    int16_t origin[BFM_HERO_SMOKE_ORIGINS][3];
    int16_t left, top;
    uint8_t width, height;
    BfmHeroSmokeFrame tracks[BFM_HERO_SMOKE_TRACKS][BFM_HERO_SMOKE_CELLS];
} BfmHeroSmokeMaterial;

/* scene is expanded PAC data from the user's mounted SC01.CD member 1. */
int bfm_hero_smoke_load(BfmHeroSmokeMaterial *out, const uint8_t *scene, size_t bytes);

/* field_age starts at the authored animation's first cell. In the reveal,
 * a newly visible particle has already advanced one cell on track zero.
 * The returned frame borrows the material; NULL means the track has ended. */
const BfmHeroSmokeFrame *bfm_hero_smoke_frame(const BfmHeroSmokeMaterial *material,
                                              unsigned track, unsigned field_age);
#endif
