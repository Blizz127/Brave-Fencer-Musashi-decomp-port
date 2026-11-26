/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_FUSION_H
#define BFM_WOODS_FUSION_H

#include "bedroom_travel.h"
#include "forest_flight.h"
#include "forest_tether_trails.h"
#include "gameplay_run.h"

typedef struct BfmWoodsCaster BfmWoodsCaster;
typedef struct BfmWoodsFusion BfmWoodsFusion;

typedef struct {
    int controllable, moving, shoulder, square_edge;
    uint32_t visible; /* The current equipped body parts, including Lumina. */
} BfmWoodsFusionInput;

typedef struct {
    uint32_t pose, visible; /* pose is the resident joint packet, after its 16-byte header. */
    int action, blade_away, movement_locked, tether_gauge, open_award;
    const BfmForestActor *blade;
    const BfmForestTetherTrails *trails;
    const BfmFusionFlight *flight;
} BfmWoodsFusionFrame;

BfmWoodsFusion *bfm_woods_fusion_open(const MusashiDiscMedia *disc, const uint8_t *scene,
                                     size_t bytes);
void bfm_woods_fusion_free(BfmWoodsFusion *fusion);
/* Advance once per active field update. A catch raises open_award once for
 * the mounted reader. Movement during a shoulder stance remains available.
 * All rendering pointers borrow state until the next update. */
int bfm_woods_fusion_step(BfmWoodsFusion *fusion, BfmWoodsCaster *casters, BfmGameplayRun *run,
                          BfmForestStatus *status, const BfmBedroomRoot *root, unsigned field_tick,
                          const BfmWoodsFusionInput *input, BfmWoodsFusionFrame *frame);
/* Call after the final page has closed; the normal release clip then equips
 * the ability on the player's existing status record. */
int bfm_woods_fusion_close_award(BfmWoodsFusion *fusion);

#endif
