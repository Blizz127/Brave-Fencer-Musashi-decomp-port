/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_LARDWICK_ART_H
#define BFM_WOODS_LARDWICK_ART_H

#include "bedroom_figure.h"
#include "woods_lardwick_field.h"

typedef struct BfmWoodsLardwickArt BfmWoodsLardwickArt;

/* Mount the crystal, Soldier and both rescue clips on entry to the occupied
 * field. Keep the room's mounted scene available while using its art. */
BfmWoodsLardwickArt *bfm_woods_lardwick_art_load(const uint8_t *scene, size_t bytes,
                                                const BfmWoodsLardwickField *field);
void bfm_woods_lardwick_art_free(BfmWoodsLardwickArt *art);
int bfm_woods_lardwick_art_add(const BfmWoodsLardwickArt *art, BfmBedroomFigure *figure,
                              const BfmBedroomView *view, const BfmWoodsLardwickField *field,
                              unsigned field_tick);
#endif
