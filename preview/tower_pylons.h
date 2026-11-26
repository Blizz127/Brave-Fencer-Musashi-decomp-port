/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_PYLONS_H
#define BFM_TOWER_PYLONS_H

#include "forest_guard_contact.h"
#include "gameplay_run.h"

enum { BFM_TOWER_POSTS = 4 };
enum BfmTowerPlatePhase { BFM_TOWER_COLUMN, BFM_TOWER_FRACTURE, BFM_TOWER_EXPOSED,
                          BFM_TOWER_PRESSING, BFM_TOWER_CREDITED };

typedef struct {
    enum BfmTowerPlatePhase phase;
    unsigned remaining, ticks, plate_yaw;
    int16_t fragment[3], shake_x;
} BfmTowerPylonFrame;

typedef struct {
    BfmTowerPylonFrame frame;
    BfmForestTransform body;
    int16_t home[3], shake_offset[3];
    unsigned last_stroke, note_delay;
    int fresh_hit;
    int32_t fragment_velocity[3], fragment_distance[3];
} BfmTowerPylon;

typedef struct {
    BfmTowerPylon post[BFM_TOWER_POSTS];
    BfmForestGuardBox box;
    unsigned note_started, note_closed, pressing, credits, next_frame;
    BfmTowerPylonFrame history[3][BFM_TOWER_POSTS];
} BfmTowerPylons;

int bfm_tower_pylons_load(BfmTowerPylons *pylons, const uint8_t *scene, size_t bytes);
void bfm_tower_pylons_tick(BfmTowerPylons *pylons, const int16_t player[3], BfmGameplayRun *gameplay);
int bfm_tower_pylons_hit(BfmTowerPylons *pylons, unsigned stroke_id, const int16_t start[3],
                         const int16_t end[3], int16_t contact[3], int16_t normal[3]);
int bfm_tower_pylons_slide(const BfmTowerPylons *pylons, const int16_t from[3],
                           const int16_t to[3], int16_t result[3]);
unsigned bfm_tower_pylons_broken(const BfmTowerPylons *pylons);
unsigned bfm_tower_lit_nodes(const BfmTowerPylonFrame posts[BFM_TOWER_POSTS]);

#endif
