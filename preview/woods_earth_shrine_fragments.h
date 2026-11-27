/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_EARTH_SHRINE_FRAGMENTS_H
#define BFM_WOODS_EARTH_SHRINE_FRAGMENTS_H

#include "bedroom_figure.h"
#include "gameplay_run.h"

typedef struct BfmWoodsEarthShrineFragments BfmWoodsEarthShrineFragments;

BfmWoodsEarthShrineFragments *bfm_woods_earth_shrine_fragments_open(
    const uint8_t *scene, size_t bytes);
void bfm_woods_earth_shrine_fragments_free(BfmWoodsEarthShrineFragments *fragments);

/* The genuine child detachment scatters the tablet once. Supply the current
 * tablet parent coordinates and heading; the loose scroll has a different
 * origin. The shared run supplies three successive spin draws per fragment. */
int bfm_woods_earth_shrine_fragments_release(BfmWoodsEarthShrineFragments *fragments,
    const int16_t parent_xyz[3], unsigned parent_yaw, BfmGameplayRun *run);
void bfm_woods_earth_shrine_fragments_step(BfmWoodsEarthShrineFragments *fragments,
    int parent_y);
int bfm_woods_earth_shrine_fragments_draw(const BfmWoodsEarthShrineFragments *fragments,
    BfmBedroomFigure *figure, const BfmBedroomView *view);

#endif
