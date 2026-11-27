/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_3026_BOX_ART_H
#define BFM_WOODS_3026_BOX_ART_H
#include "bedroom_figure.h"

typedef struct BfmWoods3026BoxArt BfmWoods3026BoxArt;
enum { BFM_WOODS_3026_BOX_OPEN = 32 };

/* SC02.CD member 11. The decoded models and pose keys outlive the scene. */
BfmWoods3026BoxArt *bfm_woods_3026_box_art_load(const uint8_t *scene, size_t bytes);
void bfm_woods_3026_box_art_free(BfmWoods3026BoxArt *art);
/* Append both pieces to the prepared room figure queue. Zero selects the
 * closed pose. Opening steps forward one key per tick; closing retraces it. */
int bfm_woods_3026_box_art_draw(const BfmWoods3026BoxArt *art, BfmBedroomFigure *figure,
                               const BfmBedroomView *view, unsigned openness);
#endif
