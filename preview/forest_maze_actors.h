/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_MAZE_ACTORS_H
#define BFM_FOREST_MAZE_ACTORS_H

#include "bedroom_travel.h"
#include "village_ground.h"

typedef struct BfmForestMazeActors BfmForestMazeActors;

/* Borrows the selected SC03/7 field and its collision map throughout this visit.
 * Actor initialization reads the proposed entry save without changing it. */
BfmForestMazeActors *bfm_forest_maze_actors_open(const uint8_t *scene, size_t bytes,
    unsigned entrance, const BfmVillageGround *ground, const BfmBedroomRoot *player,
    const BfmSaveRecord *save);
void bfm_forest_maze_actors_free(BfmForestMazeActors *actors);
/* A positive value selects the guide's field sound on this update. */
unsigned bfm_forest_maze_actors_step(BfmForestMazeActors *actors, const BfmBedroomRoot *player,
                                     BfmSaveRecord *save, BfmGameplayRun *run);
/* The borrowed list is valid until the next draw. A valid empty list has count zero. */
const BfmForestActor *bfm_forest_maze_actors_draw(BfmForestMazeActors *actors, size_t *count);
/* Optional sprite and supporting surface of an actor in the current draw list. */
const uint8_t *bfm_forest_maze_actor_shadow(const BfmForestMazeActors *actors,
                                          size_t actor_index, BfmBedroomRoot *ground,
                                          unsigned *size);
/* Ordinary foot contact and Square interaction with the sign in the alternate clearing.
 * The message offset is relative to the mounted field, including its control prefix. */
int bfm_forest_maze_sign_slide(const BfmForestMazeActors *actors,
                              const int16_t before[3], int16_t endpoint[3]);
int bfm_forest_maze_sign_message(const BfmForestMazeActors *actors,
                                 const int16_t player[3], size_t *message_offset);
#endif
