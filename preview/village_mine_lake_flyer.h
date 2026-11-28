/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_LAKE_FLYER_H
#define BFM_VILLAGE_MINE_LAKE_FLYER_H
#include "bedroom_figure.h"
#include "gameplay_run.h"
#include "village_ground.h"

typedef struct BfmMineLakeFlyers BfmMineLakeFlyers;
enum { BFM_MINE_LAKE_FLYERS = 12 };

/* Mount the fixed patrol placements and their model from SC03/110.
 * The caller retains the expanded scene and its room terrain. */
BfmMineLakeFlyers *bfm_mine_lake_flyers_open(const uint8_t *scene, size_t bytes,
                                            const BfmVillageGround *ground);
void bfm_mine_lake_flyers_free(BfmMineLakeFlyers *flyers);
void bfm_mine_lake_flyers_enter(BfmMineLakeFlyers *flyers, BfmGameplayRun *run);

/* Dispatch after the player's physical field update. Each returned control is
 * a request for room command 0x6fe, in original actor order. */
unsigned bfm_mine_lake_flyers_step(BfmMineLakeFlyers *flyers, BfmGameplayRun *run,
    const int16_t hero[3], uint16_t controls[BFM_MINE_LAKE_FLYERS]);
int bfm_mine_lake_flyers_draw(const BfmMineLakeFlyers *flyers, BfmBedroomFigure *figure,
                              const BfmBedroomView *view);
#endif
