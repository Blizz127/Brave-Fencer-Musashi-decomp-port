/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_STOCKS_PROP_H
#define BFM_VILLAGE_STOCKS_PROP_H

#include "bedroom_figure.h"
#include "bedroom_record.h"
#include "gameplay_run.h"

typedef struct BfmVillageStocksProp BfmVillageStocksProp;

/* Leno is a separate field actor beside Jon. The mounted scene must outlive
 * him. A visit on which the dog is absent still returns a valid object. */
BfmVillageStocksProp *bfm_village_stocks_prop_open(const uint8_t *scene, size_t bytes,
                                                  const BfmSaveRecord *save);
void bfm_village_stocks_prop_free(BfmVillageStocksProp *dog);

/* First greeting's scene cue tells Leno to move to his speaking station.
 * A repeat conversation with no provisions leaves his current pose alone. */
void bfm_village_stocks_prop_greet(BfmVillageStocksProp *dog);
/* Signal Jon's departure cue. Returns zero when Leno is absent. */
int bfm_village_stocks_prop_send(BfmVillageStocksProp *dog);
int bfm_village_stocks_prop_step(BfmVillageStocksProp *dog, BfmGameplayRun *run);
int bfm_village_stocks_prop_add(const BfmVillageStocksProp *dog, BfmBedroomFigure *figure,
                               const BfmBedroomView *view);

#endif
