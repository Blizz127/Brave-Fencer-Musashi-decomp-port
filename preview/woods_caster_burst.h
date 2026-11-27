/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_CASTER_BURST_H
#define BFM_WOODS_CASTER_BURST_H
#include "bedroom_figure.h"
#include "gameplay_run.h"

typedef struct BfmWoodsCasterBurst BfmWoodsCasterBurst;

/* Borrow the mounted room and the resident player payload for the room visit. */
BfmWoodsCasterBurst *bfm_woods_caster_burst_open(const uint8_t *scene, size_t bytes,
                                                 const uint8_t *main, size_t main_bytes);
void bfm_woods_caster_burst_free(BfmWoodsCasterBurst *burst);
/* Tick earlier children before starting a new caster death. The caster's
 * root and heading are captured on the first death update. */
int bfm_woods_caster_burst_start(BfmWoodsCasterBurst *burst, const BfmBedroomRoot *caster,
                                 BfmGameplayRun *run);
int bfm_woods_caster_burst_tick(BfmWoodsCasterBurst *burst, BfmGameplayRun *run);
int bfm_woods_caster_burst_add(const BfmWoodsCasterBurst *burst, BfmBedroomFigure *figure,
                               const BfmBedroomView *view);
#endif
