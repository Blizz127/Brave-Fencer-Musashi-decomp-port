/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_MAZE_GUIDE_START_H
#define BFM_FOREST_MAZE_GUIDE_START_H

#include "bedroom_figure.h"

typedef struct BfmForestMazeGuideStart BfmForestMazeGuideStart;

/* The first bank owns Leno from its room placement until he runs off the map.
 * Quest variable 204 belongs to the playthrough and survives room changes. */
BfmForestMazeGuideStart *bfm_forest_maze_guide_start_open(const uint8_t *scene, size_t bytes);
void bfm_forest_maze_guide_start_free(BfmForestMazeGuideStart *guide);
uint8_t bfm_forest_maze_guide_start_step(BfmForestMazeGuideStart *guide,
                                        const int16_t player[3], uint8_t quest);
int bfm_forest_maze_guide_start_draw(const BfmForestMazeGuideStart *guide,
                                    BfmBedroomFigure *figure, const BfmBedroomView *view);
#endif
