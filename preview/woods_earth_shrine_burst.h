/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_EARTH_SHRINE_BURST_H
#define BFM_WOODS_EARTH_SHRINE_BURST_H

#include "bedroom_figure.h"

enum { BFM_EARTH_BURST_RAYS = 16, BFM_EARTH_BURST_UPDATES = 15 };

typedef struct {
    int16_t direction[2], rotation[2];
    unsigned radius, length;
} BfmWoodsEarthShrineRay;

typedef struct {
    BfmWoodsEarthShrineRay rays[BFM_EARTH_BURST_RAYS];
    int16_t center[2];
    unsigned remaining;
} BfmWoodsEarthShrineBurst;

/* Zero-initialize per visit. Only the primary child's actual detach starts
 * the burst. Pass its projected screen coordinates and the live scene RNG. */
int bfm_woods_earth_shrine_burst_start(BfmWoodsEarthShrineBurst *burst,
    const int projected_xy[2], unsigned (*random)(void *), void *context);

/* Once per scheduled burst update, including the detach update, before drawing.
 * Repeated draws of the same field reuse the same projected center and rays. */
void bfm_woods_earth_shrine_burst_step(BfmWoodsEarthShrineBurst *burst);
int bfm_woods_earth_shrine_burst_append(const BfmWoodsEarthShrineBurst *burst,
    BfmBedroomFigure *figure, const BfmBedroomView *view);

#endif
