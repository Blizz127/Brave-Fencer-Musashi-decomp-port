/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_INTERIOR_FIXTURES_H
#define BFM_TOWER_INTERIOR_FIXTURES_H

#include "forest_dust.h"
#include "gameplay_run.h"
#include <stddef.h>

/* The entrance camera and first follow shot need at most 40 visible cards
 * across the complete range of hanging heights. Reserve room for a moving view. */
enum { BFM_TOWER_HANGING_LIGHTS = 4, BFM_TOWER_FIXTURE_CARDS = 64 };

typedef struct {
    int16_t root[3];
    uint16_t phase;
    unsigned style;
} BfmTowerLight;

typedef struct {
    BfmForestRasterMaterial ink;
    int16_t left, top;
    uint8_t u, v, width, height;
} BfmTowerLightSprite;

typedef struct {
    BfmTowerLight light[BFM_TOWER_HANGING_LIGHTS];
    BfmTowerLightSprite sprite[2];
    unsigned count;
    int started;
} BfmTowerInteriorFixtures;

int bfm_tower_interior_fixtures_load(const uint8_t *scene, size_t bytes,
                                     BfmTowerInteriorFixtures *fixtures);
void bfm_tower_interior_fixtures_start(BfmTowerInteriorFixtures *fixtures, BfmGameplayRun *run);
/* Count full actor updates since start, including updates while text owns control. */
size_t bfm_tower_interior_fixture_cards(const BfmTowerInteriorFixtures *fixtures, unsigned age,
                                        const BfmForestCamera *camera, BfmForestDustCard *out,
                                        size_t capacity);
#endif
