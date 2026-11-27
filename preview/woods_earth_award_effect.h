/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_EARTH_AWARD_EFFECT_H
#define BFM_WOODS_EARTH_AWARD_EFFECT_H

#include "bedroom_figure.h"
#include "gameplay_run.h"

typedef struct BfmWoodsEarthAwardEffect BfmWoodsEarthAwardEffect;
typedef struct {
    int message_ready;
    unsigned sound_count, sounds[30];
} BfmEarthAwardEffectFrame;

/* Read the Earth ceremony art from the mounted user disc. The scene can be
 * released after open. No field texture or effect changes until spawn. */
BfmWoodsEarthAwardEffect *bfm_woods_earth_award_effect_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes);
void bfm_woods_earth_award_effect_free(BfmWoodsEarthAwardEffect *effect);
/* One advancing field, including dialogue. Spawn on the player pose marker,
 * passing its model root, current camera and mounted tiles. Finish on actual
 * message dismissal. Capture the message pulse and route the listed cues. */
int bfm_woods_earth_award_effect_step(BfmWoodsEarthAwardEffect *effect, BfmGameplayRun *run,
    int spawn, int finish, const BfmForestTransform *hero, const BfmBedroomView *view,
    uint16_t tiles[512][1024], BfmEarthAwardEffectFrame *out);
/* Submit the last completed field. Repeated draws do not advance effects. */
int bfm_woods_earth_award_effect_add(const BfmWoodsEarthAwardEffect *effect,
    BfmBedroomFigure *figure, const BfmBedroomView *view);
#endif
