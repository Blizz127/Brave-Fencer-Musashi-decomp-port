/* SPDX-License-Identifier: MIT */
#ifndef BFM_VALLEY_QUAKE_ART_H
#define BFM_VALLEY_QUAKE_ART_H

#include "bedroom_figure.h"

typedef struct {
    int16_t ground[3];
    unsigned yaw, age;
    int scale[3];
} BfmValleyQuakeArt;

/* Zero-initialize on field entry. Advance once per field update, including the
 * grounded Earth pulse. Return one on each child allocation attempt for its
 * sound cue. The waves keep the position and facing at impact. */
int bfm_valley_quake_art_step(BfmValleyQuakeArt *art, int quake,
                              const int16_t ground[3], unsigned yaw);
int bfm_valley_quake_art_draw(const BfmValleyQuakeArt *art, BfmBedroomFigure *figure,
                              const BfmBedroomView *view);

#endif
