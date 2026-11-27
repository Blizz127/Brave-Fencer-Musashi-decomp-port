/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_AWARD_EFFECT_H
#define BFM_WOODS_AWARD_EFFECT_H

#include "bedroom_figure.h"

enum { BFM_WOODS_AWARD_ECHOES = 8 };
typedef struct {
    int scale, light;
    unsigned remaining;
} BfmWoodsAwardEcho;

typedef struct {
    BfmForestDustCard picture;
    BfmWoodsAwardEcho echoes[BFM_WOODS_AWARD_ECHOES];
    unsigned phase, counter, hold;
    int scale, speed, light;
} BfmWoodsAwardEffect;

/* Zero-initialize on room entry. Only a positive earned-award release edge
 * loads Shrink's picture into its allocated skill slot. */
int bfm_woods_award_effect_step(BfmWoodsAwardEffect *effect, const MusashiDiscMedia *disc,
                               const uint8_t *scene, size_t bytes, uint16_t tiles[512][1024],
                               int release_award);
/* Pass the earned Satiate (3) or Shrink (13) on the release pose key.
 * A zero advances the current picture and its echoes by one field. */
int bfm_woods_award_effect_step_ability(BfmWoodsAwardEffect *effect, const MusashiDiscMedia *disc,
                                       const uint8_t *scene, size_t bytes, uint16_t tiles[512][1024],
                                       unsigned released_ability);
int bfm_woods_award_effect_append(const BfmWoodsAwardEffect *effect, BfmBedroomFigure *figure,
                                  const BfmBedroomView *view);
#endif
