/* SPDX-License-Identifier: MIT */
#ifndef BFM_INN_KEEPER_H
#define BFM_INN_KEEPER_H

#include "bedroom_figure.h"

typedef struct BfmInnKeeper BfmInnKeeper;

/* The lower village and the inn member remain mounted for the keeper's lifetime. */
BfmInnKeeper *bfm_inn_keeper_load(const uint8_t *scene, size_t bytes);
BfmInnKeeper *bfm_vambee_inn_keeper_load(const uint8_t *scene, size_t bytes);
void bfm_inn_keeper_free(BfmInnKeeper *keeper);
int bfm_inn_keeper_add(const BfmInnKeeper *keeper, BfmBedroomFigure *figure,
                       const BfmBedroomView *view, unsigned age);

#endif
