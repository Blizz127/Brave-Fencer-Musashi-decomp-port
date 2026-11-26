/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_TETHER_TRAILS_H
#define BFM_FOREST_TETHER_TRAILS_H
#include "forest_dust.h"

enum { BFM_TETHER_STREAMS = 8, BFM_TETHER_SAMPLES = 16,
       BFM_TETHER_CARDS = BFM_TETHER_STREAMS * (BFM_TETHER_SAMPLES - 1) * 2 };

typedef struct {
    int16_t edge[2][3];
} BfmTetherSample;

typedef struct {
    int64_t position[3];
    int32_t velocity[3];
    BfmTetherSample samples[BFM_TETHER_SAMPLES];
    unsigned phase, ticks, head, links, light;
} BfmTetherStream;

typedef struct {
    BfmTetherStream streams[BFM_TETHER_STREAMS];
    int16_t origin[3];
    unsigned yaw, last_meter, cooldown;
    int enabled;
} BfmForestTetherTrails;

typedef unsigned (*BfmTetherRandom)(void *context);

/* Start on a confirmed Fusion latch. target is the struck actor's root;
 * blade is the attached Fusion projectile. Both are in field world units. */
void bfm_forest_tether_trails_begin(BfmForestTetherTrails *trails,
                             const int16_t target[3], const int16_t blade[3]);

/* Tick once per subsequent field update, including the return flight. The
 * caller checks the original bound actor generation and assimilation drive
 * flag to supply driven. A missing blade sends the surviving trails toward
 * the player; an increased meter can emit before the ordinary cooldown. */
void bfm_forest_tether_trails_tick(BfmForestTetherTrails *trails, int driven, unsigned meter,
                            const int16_t blade[3], const int16_t player[3],
                            BfmTetherRandom random, void *context);

/* Append projected additive quads to the room's shared actor/terrain draw.
 * capacity is the remaining space in the caller's effect array. */
size_t bfm_forest_tether_trails_project(const BfmForestTetherTrails *trails,
                 const BfmForestCamera *camera, BfmForestDustCard *out, size_t capacity);
#endif
