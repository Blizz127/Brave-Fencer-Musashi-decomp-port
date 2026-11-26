/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_BRIDGE_H
#define BFM_FOREST_BRIDGE_H
#include "forest_bridge_targets.h"
#include "forest_bridge_floor.h"

typedef enum {
    BFM_LOG_UNPLACED, BFM_LOG_STANDING, BFM_LOG_FALLING, BFM_LOG_SETTLED
} BfmForestLogState;

typedef struct {
    BfmForestLogState state;
    uint32_t flags;
    int threshold, angle, velocity, pitch, bounces;
} BfmForestLog;

typedef struct {
    BfmForestBridgeTargets *targets;
    BfmForestBridgeFloor floor;
    BfmForestLog logs[BFM_FOREST_BRIDGE_TARGET_COUNT];
    unsigned counter, phase;
} BfmForestBridge;

/* Advance the hinge after a room log's own activation delay. The caller
 * applies the resulting model pitch to its specific mounted placement. */
void bfm_forest_log_fall(BfmForestLog *log);
int bfm_forest_bridge_load(BfmForestBridge *bridge, const BfmForestField *field);
void bfm_forest_bridge_approach(BfmForestBridge *bridge, int z);
unsigned bfm_forest_bridge_advance(BfmForestBridge *bridge, unsigned lesson_phase);
unsigned bfm_forest_bridge_contact(BfmForestBridge *bridge, const int16_t from[3],
                                   const int16_t to[3], int16_t hit[3], int16_t normal[3]);
int bfm_forest_bridge_support(const BfmForestBridge *bridge, BfmForestBridgeProbe query,
                              const int16_t from[3], const int16_t to[3], BfmForestContact *floor);
#endif
