/* SPDX-License-Identifier: MIT */
#ifndef BFM_DIALOGUE_CONTOUR_H
#define BFM_DIALOGUE_CONTOUR_H
#include <stdint.h>

enum { BFM_DIALOGUE_CORNERS = 30 };
typedef struct { int x, y; } BfmDialoguePoint;

void bfm_dialogue_contour(const BfmDialoguePoint border[BFM_DIALOGUE_CORNERS],
                         const BfmDialoguePoint inset[BFM_DIALOGUE_CORNERS],
                         uint32_t paper, uint32_t highlight, uint32_t outline,
                         int top, uint32_t pixels[240][320]);
#endif
