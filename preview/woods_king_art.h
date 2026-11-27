/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_KING_ART_H
#define BFM_WOODS_KING_ART_H
#include "bedroom_figure.h"

typedef struct BfmWoodsKingArt BfmWoodsKingArt;
typedef enum { BFM_KING_IDLE, BFM_KING_BITE, BFM_KING_HURT } BfmWoodsKingPose;

/* The original 3025 field member stays mounted for the art's lifetime. */
BfmWoodsKingArt *bfm_woods_king_art_open(const uint8_t *scene, size_t bytes);
void bfm_woods_king_art_free(BfmWoodsKingArt *art);
/* elapsed starts at zero on clip entry; drawing does not advance the actor.
 * The root includes this frame's ground projection for the separate shadow. */
int bfm_woods_king_art_add(BfmWoodsKingArt *art, BfmBedroomFigure *figure, const BfmBedroomView *view,
                           const BfmBedroomRoot *root, BfmWoodsKingPose pose, unsigned elapsed,
                           unsigned horizontal_q12, unsigned vertical_q12);
#endif
