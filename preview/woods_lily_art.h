/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_LILY_ART_H
#define BFM_WOODS_LILY_ART_H

#include "bedroom_figure.h"

typedef struct BfmWoodsLilyArt BfmWoodsLilyArt;

/* The three Cleft placements share a single mounted model and local pose. */
BfmWoodsLilyArt *bfm_woods_lily_art_load(const uint8_t *scene, size_t bytes);
void bfm_woods_lily_art_free(BfmWoodsLilyArt *art);

/* Append one pad to the prepared field figure queue. Its root includes the
 * platform's current movement and tilt; the mesh has its own local offset. */
int bfm_woods_lily_art_append(const BfmWoodsLilyArt *art, BfmBedroomFigure *figure,
                             const BfmBedroomView *view, const BfmForestTransform *root);

#endif
