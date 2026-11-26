/* SPDX-License-Identifier: MIT */
#ifndef BFM_ROOM_WALK_GRID_H
#define BFM_ROOM_WALK_GRID_H

#include <stddef.h>
#include <stdint.h>

typedef struct BfmRoomWalkGrid BfmRoomWalkGrid;

BfmRoomWalkGrid *bfm_room_walk_grid_open(const uint8_t *scene, size_t bytes, uint32_t room);
void bfm_room_walk_grid_free(BfmRoomWalkGrid *walk);

/* Resolve one grounded step on unmarked level polygons, clipping against
 * bounded walls in the mounted room. A refused step leaves to unchanged. */
int bfm_room_walk_grid_level_step(const BfmRoomWalkGrid *walk, const int16_t from[3],
                                  int16_t to[3]);

#endif
