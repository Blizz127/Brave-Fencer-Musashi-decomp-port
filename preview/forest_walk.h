/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_WALK_H
#define BFM_FOREST_WALK_H
#include "forest_entry.h"

typedef struct BfmForestWalk BfmForestWalk;

/* The door owns this short automatic walk. Caller hands over the transfer
 * resolved from its active opening scene when the chapter card has finished. */
BfmForestWalk *bfm_forest_walk_open(const MusashiDiscMedia *disc, const RoomTransferDescriptor *entry);
void bfm_forest_walk_free(BfmForestWalk *walk);
/* 1 advances one field update, 0 stops at the next unsupported scene event,
 * -1 indicates bad mounted data. The first frame can be drawn immediately. */
int bfm_forest_walk_tick(BfmForestWalk *walk);
int bfm_forest_walk_tick_input(BfmForestWalk *walk, int confirm_edge);
int bfm_forest_walk_tick_axes(BfmForestWalk *walk, int x, int y, int confirm_edge);
/* 0 during the door event, 1 under player control, 2 at the preview boundary. */
int bfm_forest_walk_control(const BfmForestWalk *walk);
struct ForestVoice;
struct ForestVoice *bfm_forest_walk_voice(BfmForestWalk *walk, unsigned *generation);
int bfm_forest_walk_draw(BfmForestWalk *walk, uint32_t pixels[240][320]);
#endif
