/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_LAKE_VIEW_H
#define BFM_VILLAGE_MINE_LAKE_VIEW_H

#include "village_mine_view.h"

typedef struct {
    BfmVillageMineView *view;
    uint16_t (*tiles)[1024];
    const uint8_t *transfers[2];
    int zoom_radius[2], radius;
    unsigned zoom;
    int phase;
} BfmVillageMineLakeView;

/* The room member and shared texture pages stay mounted through close. */
int bfm_village_mine_lake_view_open(BfmVillageMineLakeView *lake, const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, uint16_t tiles[512][1024]);
void bfm_village_mine_lake_view_close(BfmVillageMineLakeView *lake);
/* Follow at entry and after player movement. Either newly pressed shoulder toggles zoom. */
void bfm_village_mine_lake_view_follow(BfmVillageMineLakeView *lake, const int16_t foot[3],
                                      unsigned pressed);
/* Advance once per room update, independently of drawing or camera movement. */
void bfm_village_mine_lake_view_tick_tiles(BfmVillageMineLakeView *lake);

#endif
