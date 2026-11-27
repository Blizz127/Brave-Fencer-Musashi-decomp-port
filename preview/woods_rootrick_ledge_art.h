/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_ROOTRICK_LEDGE_ART_H
#define BFM_WOODS_ROOTRICK_LEDGE_ART_H

#include "bedroom_figure.h"

typedef struct BfmRootrickWall BfmRootrickWall;
typedef struct BfmWoodsRootrickLedgeArt BfmWoodsRootrickLedgeArt;

/* Mount both ledge models from SC02/11. The wall outlives this renderer. */
BfmWoodsRootrickLedgeArt *bfm_woods_rootrick_ledge_art_open(const uint8_t *scene, size_t bytes,
                                                            const BfmRootrickWall *wall);
void bfm_woods_rootrick_ledge_art_free(BfmWoodsRootrickLedgeArt *art);
/* The three platforms remain drawable while retracted into the wall. */
int bfm_woods_rootrick_ledge_art_add(const BfmWoodsRootrickLedgeArt *art, BfmBedroomFigure *figure,
                                    const BfmBedroomView *view);

#endif
