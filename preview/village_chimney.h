/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_CHIMNEY_H
#define BFM_VILLAGE_CHIMNEY_H

#include "bedroom_figure.h"
#include "gameplay_run.h"

typedef struct BfmVillageChimney BfmVillageChimney;

/* The lower street keeps its mounted scene alive for the emitter's lifetime. */
BfmVillageChimney *bfm_village_chimney_open(const MusashiDiscMedia *disc,
                                            const uint8_t *scene, size_t bytes);
void bfm_village_chimney_free(BfmVillageChimney *chimney);
void bfm_village_chimney_step(BfmVillageChimney *chimney, BfmGameplayRun *run, unsigned hour);
int bfm_village_chimney_add(const BfmVillageChimney *chimney, BfmBedroomFigure *figure,
                            const BfmBedroomView *view);

#endif
