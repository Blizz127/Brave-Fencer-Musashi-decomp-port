/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_SATIATE_EFFECT_H
#define BFM_WOODS_SATIATE_EFFECT_H

#include "bedroom_figure.h"
#include "gameplay_run.h"

typedef struct BfmWoodsSatiateEffect BfmWoodsSatiateEffect;

BfmWoodsSatiateEffect *bfm_woods_satiate_effect_load(const MusashiDiscMedia *disc);
void bfm_woods_satiate_effect_free(BfmWoodsSatiateEffect *effect);
/* Once per advancing room effects update, including dialogue. Pass the room's
 * field counter for sound cadence and pulse armed when the passive starts.
 * Tiles are uploaded on arm, including when re-entering with Satiate.
 * The eye is sampled only when a food picture reaches Musashi. */
unsigned bfm_woods_satiate_effect_tick(BfmWoodsSatiateEffect *effect, BfmGameplayRun *run,
    const int16_t player[3], const int32_t eye[3], unsigned field_tick,
    unsigned equipped, int armed, uint16_t tiles[512][1024]);
int bfm_woods_satiate_effect_add(const BfmWoodsSatiateEffect *effect,
    BfmBedroomFigure *figure, const BfmBedroomView *view);
/* Independent multiplicative animation factors; neutral is 4096 per axis. */
void bfm_woods_satiate_effect_scale(const BfmWoodsSatiateEffect *effect, int xyz_q12[3]);

#endif
