/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_SHAFT_VIEW_H
#define BFM_VILLAGE_MINE_SHAFT_VIEW_H

#include "village_mine_view.h"

/* The mine shaft keeps the mounted SC03/109 scene and tiles for its lifetime. */
BfmVillageMineView *bfm_village_mine_shaft_view_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, uint16_t tiles[512][1024]);
void bfm_village_mine_shaft_view_follow(BfmVillageMineView *view, const int16_t foot[3]);

#endif
