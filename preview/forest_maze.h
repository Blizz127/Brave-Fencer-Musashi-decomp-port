/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_MAZE_H
#define BFM_FOREST_MAZE_H

#include "field_input.h"
#include "forest_message.h"
#include "village_arrival_motion.h"

typedef struct BfmForestMaze BfmForestMaze;

/* The caller prepares the source departure on a copy of the save. Opening
 * this visit then installs the receiving entrance's actor state on that copy. */
BfmForestMaze *bfm_forest_maze_open(const MusashiDiscMedia *disc, const uint8_t *main,
                                    size_t main_bytes, BfmSaveRecord *incoming);
void bfm_forest_maze_free(BfmForestMaze *maze);
BfmVillageMotion *bfm_forest_maze_motion(BfmForestMaze *maze);
int bfm_forest_maze_step(BfmForestMaze *maze, BfmFieldInput field, BfmGameplayRun *run,
                         BfmSaveRecord *save, BfmForestMessage **opened);
int bfm_forest_maze_departing(BfmForestMaze *maze, BfmGameplayRun *run, BfmSaveRecord *save);
void bfm_forest_maze_close_sign(BfmForestMaze *maze);
unsigned bfm_forest_maze_take_sound(BfmForestMaze *maze);
int bfm_forest_maze_draw(BfmForestMaze *maze, const uint8_t flags[64],
                         const BfmForestMessageFrame *window, uint32_t pixels[240][320]);

#endif
