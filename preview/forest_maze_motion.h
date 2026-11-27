/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_MAZE_MOTION_H
#define BFM_FOREST_MAZE_MOTION_H

#include "forest_bank_hop_ring.h"
#include "village_arrival_motion.h"
#include "village_camera.h"

typedef struct BfmForestMazeMotion BfmForestMazeMotion;

BfmForestMazeMotion *bfm_forest_maze_motion_open(const MusashiDiscMedia *disc,
                                                  const uint8_t *scene, size_t bytes);
void bfm_forest_maze_motion_free(BfmForestMazeMotion *maze);
/* One eligible player update. During the walk-in the route owns input.
 * Resuming an equipped Hop on release retains the saved timer and BP. */
int bfm_forest_maze_motion_step(BfmForestMazeMotion *maze, BfmVillageMotion *motion,
    struct BfmGameplayRun *run, BfmFieldInput field, int camera_yaw, unsigned *sound);
int16_t bfm_forest_maze_tracking_y(const BfmForestMazeMotion *maze, int16_t foot_y);
/* Advance after the camera on each actor field, then project into the room's
 * existing sorted card list when drawing. */
void bfm_forest_maze_motion_effects(BfmForestMazeMotion *maze, const BfmVillageCamera *camera);
unsigned bfm_forest_maze_motion_cards(const BfmForestMazeMotion *maze,
    const BfmVillageCamera *camera, BfmForestDustCard *cards, unsigned capacity);

#endif
