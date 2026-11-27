/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_CHARGED_FX_H
#define BFM_WOODS_CHARGED_FX_H

#include "forest_sweep.h"

typedef struct BfmWoodsChargedFx BfmWoodsChargedFx;

/* Two mounted wave models, twelve annulus sectors, and the Lumina swing ribbon. */
enum { BFM_WOODS_CHARGED_CARDS = 2 * 96 + 12 + BFM_FOREST_SWEEP_CARDS,
       BFM_WOODS_CHARGED_SOUND = 2522 };

BfmWoodsChargedFx *bfm_woods_charged_fx_open(const uint8_t *scene, size_t scene_bytes,
                                             const uint8_t *main, size_t main_bytes);
void bfm_woods_charged_fx_free(BfmWoodsChargedFx *fx);
/* Step once per active effect field, including fields after the move ends.
 * The edge holds the logical Lumina pose's world tip followed by its guard.
 * entry and sync are the actual action-entry and mounted pose marker edges. */
int bfm_woods_charged_fx_step(BfmWoodsChargedFx *fx, int entry, int sync,
                              int swing_active, const BfmForestSweepEdge *edge);
/* Valid only on fields where the primary wave actually tests targets. */
int bfm_woods_charged_fx_wave_center(const BfmWoodsChargedFx *fx, int16_t xyz[3]);
/* The primary wave's model supplies the hit packet's heading. */
unsigned bfm_woods_charged_fx_wave_yaw(const BfmWoodsChargedFx *fx);
size_t bfm_woods_charged_fx_project(const BfmWoodsChargedFx *fx, const BfmForestCamera *camera,
                                   BfmForestDustCard cards[BFM_WOODS_CHARGED_CARDS]);
#endif
