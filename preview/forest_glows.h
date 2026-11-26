/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_GLOWS_H
#define BFM_FOREST_GLOWS_H

#include "forest_field.h"

enum { BFM_FOREST_GLOWS = 7, BFM_FOREST_ENTRY_GLOWS = 2, BFM_FOREST_GLOW_KEYS = 10 };

typedef struct {
    int16_t placement_z, start_yaw, route[2][3];
    unsigned first_target;
    int active;
    uint8_t seed_phase, target, key;
    uint16_t phase;
    int16_t yaw, pitch, jitter[3];
    int32_t position[3];
    BfmForestTransform figure;
} BfmForestGlow;

typedef struct {
    int16_t position[3];
    uint16_t phase;
} BfmForestGlowFrame;

typedef struct {
    BfmForestGlow glows[BFM_FOREST_GLOWS];
    unsigned order[BFM_FOREST_GLOWS], count;
    int started;
    int32_t forward;
    const uint8_t *halo;
    BfmForestGlowFrame frame[BFM_FOREST_GLOWS];
    unsigned draw_count;
} BfmForestGlows;

/* Mount the two ordinary room placements and the five proximity-triggered
 * placements, preserving their list order. No random state is consumed here. */
int bfm_forest_glows_load(BfmForestGlows *lights, const BfmForestField *field,
                          const uint8_t *room_descriptor);
void bfm_forest_glows_start(BfmForestGlows *lights, BfmGameplayRun *run);
/* Update existing lights after the player's movement. Then, in the field's
 * placement pass, check for new arrivals. Birth does not run a movement tick. */
void bfm_forest_glows_move(BfmForestGlows *lights, int player_z, BfmGameplayRun *run);
void bfm_forest_glows_place(BfmForestGlows *lights, int player_z, BfmGameplayRun *run);
#endif
