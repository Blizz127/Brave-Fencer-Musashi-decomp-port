/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_MAZE_VIEW_H
#define BFM_FOREST_MAZE_VIEW_H

#include "bedroom_motion.h"
#include "bedroom_travel.h"
#include "forest_dust.h"
#include "village_camera.h"

typedef struct BfmForestMazeView BfmForestMazeView;

/* First Meandering Forest fork, reached from the bank through entrance 0 or 33. All geometry and
 * materials come from the mounted disc. Keep the scene alive until view_free. */
BfmForestMazeView *bfm_forest_maze_view_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, unsigned entrance);
void bfm_forest_maze_view_free(BfmForestMazeView *view);

/* Run once after the field actor updates. Either shoulder's pressed edge
 * changes zoom. The room clamps the follow target at each exit. */
void bfm_forest_maze_view_step(BfmForestMazeView *view, const int16_t xyz[3],
                               int shoulder_pressed);
/* Borrow the room texture surface for ordinary field HUD and message art. */
uint16_t (*bfm_forest_maze_view_tiles(BfmForestMazeView *view))[1024];
/* Supply the current saved daylight phase on entry and each active field. */
int bfm_forest_maze_view_daylight(BfmForestMazeView *view, unsigned phase);
const BfmVillageCamera *bfm_forest_maze_view_camera(const BfmForestMazeView *view);

/* The caller supplies this field's ordinary body and equipment plus the
 * active room actors. Their faces and the sparse canopy share a depth sort. */
int bfm_forest_maze_view_draw(BfmForestMazeView *view, const BfmBedroomRoot *root,
    const BfmBedroomPose *pose, const uint8_t flags[64],
    const BfmForestActor *actors, size_t actor_count, uint32_t pixels[240][320]);
int bfm_forest_maze_view_draw_effects(BfmForestMazeView *view, const BfmBedroomRoot *root,
    const BfmBedroomPose *pose, const uint8_t flags[64], const BfmForestActor *actors,
    size_t actor_count, const BfmForestDustCard *cards, size_t card_count,
    uint32_t pixels[240][320]);
typedef struct {
    const BfmBedroomRoot *ground;
    const uint8_t *descriptor; /* Mounted 12-byte shadow shape. */
    unsigned size;
} BfmForestMazeShadow;

int bfm_forest_maze_view_draw_room(BfmForestMazeView *view, const BfmBedroomRoot *root,
    const BfmBedroomPose *pose, const uint8_t flags[64], const BfmForestActor *actors,
    const BfmForestMazeShadow *shadows, size_t actor_count, const BfmForestDustCard *cards,
    size_t card_count, uint32_t pixels[240][320]);
#endif
