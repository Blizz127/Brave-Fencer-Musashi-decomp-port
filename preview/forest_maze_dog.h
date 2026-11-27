/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_MAZE_DOG_H
#define BFM_FOREST_MAZE_DOG_H

#include "bedroom_travel.h"
#include "gameplay_run.h"
#include "village_ground.h"

typedef struct BfmMazeDog BfmMazeDog;
typedef enum {
    BFM_MAZE_DOG_RUN, BFM_MAZE_DOG_BRAKE, BFM_MAZE_DOG_SEARCH,
    BFM_MAZE_DOG_BARK, BFM_MAZE_DOG_REST, BFM_MAZE_DOG_CLIPS
} BfmMazeDogClip;
typedef struct {
    int16_t xyz[3];
    unsigned yaw, tick;
    BfmMazeDogClip clip;
} BfmMazeDogView;

/* Open the healthy guide from a class-118 placement in the selected roster.
 * The field owns the mounted scene, collision, save, and random stream. */
BfmMazeDog *bfm_maze_dog_open(const uint8_t *scene, size_t bytes, const BfmVillageGround *ground,
                              const int16_t center[3], const BfmBedroomRoot *player,
                              const BfmSaveRecord *save);
void bfm_maze_dog_free(BfmMazeDog *dog);
const BfmMazeDogView *bfm_maze_dog_view(const BfmMazeDog *dog);
/* Call once per field update. A nonzero return is the requested disc sound. */
unsigned bfm_maze_dog_step(BfmMazeDog *dog, const BfmBedroomRoot *player,
                           BfmSaveRecord *save, BfmGameplayRun *run);

#endif
