/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_BELL_EFFECT_H
#define BFM_TOWER_BELL_EFFECT_H

#include "forest_dust.h"
#include "gameplay_run.h"

enum { BFM_TOWER_BELL_RAYS = 32, BFM_TOWER_BELL_RAY_CARDS = 2 * BFM_TOWER_BELL_RAYS };

typedef struct {
    int16_t offsets[2][3], radius[2];
    int16_t corners[BFM_TOWER_BELL_RAYS][6][3];
    int started;
} BfmTowerBellEffect;

int bfm_tower_bell_effect_load(const uint8_t *scene, size_t size, BfmTowerBellEffect *effect);
/* Called once when the third impact interval ends. */
void bfm_tower_bell_effect_start(BfmTowerBellEffect *effect, BfmGameplayRun *run);
/* Age zero is the setup update. Ages 1 through 256 are the light phase. */
size_t bfm_tower_bell_effect_cards(const BfmTowerBellEffect *effect, unsigned age,
                                   const BfmForestCamera *camera, BfmForestDustCard *out,
                                   size_t capacity);
#endif
