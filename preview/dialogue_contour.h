/* SPDX-License-Identifier: MIT */
#ifndef BFM_DIALOGUE_CONTOUR_H
#define BFM_DIALOGUE_CONTOUR_H
#include <stdint.h>

enum { BFM_DIALOGUE_CORNERS = 30 };
typedef struct { int x, y; } BfmDialoguePoint;

BfmDialoguePoint bfm_dialogue_reflected_texel(unsigned x, unsigned y,
                                              unsigned width, unsigned height, unsigned last_v);
void bfm_dialogue_contour(const BfmDialoguePoint border[BFM_DIALOGUE_CORNERS],
                         const BfmDialoguePoint inset[BFM_DIALOGUE_CORNERS],
                         uint32_t paper, uint32_t highlight, uint32_t outline,
                         int top, uint32_t pixels[240][320]);
#endif
