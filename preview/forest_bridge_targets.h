/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_BRIDGE_TARGETS_H
#define BFM_FOREST_BRIDGE_TARGETS_H
#include "forest_field.h"

enum { BFM_FOREST_BRIDGE_TARGET_COUNT = 3 };
typedef struct BfmForestBridgeTargets BfmForestBridgeTargets;

/* Load the three placements, their shared rigid model and their standing
 * collision box from the mounted Giant Forest clearing. */
BfmForestBridgeTargets *bfm_forest_bridge_targets_open(const BfmForestField *field);
void bfm_forest_bridge_targets_free(BfmForestBridgeTargets *targets);

/* The actor and its world root are available independently of visibility.
 * Role order follows the mounted variant's high byte. The bridge simulation
 * supplies the current model pitch in units of 1/4096 turn. */
const BfmForestActor *bfm_forest_bridge_targets_actor(const BfmForestBridgeTargets *targets,
                                                      unsigned role);
void bfm_forest_bridge_targets_pitch(BfmForestBridgeTargets *targets, unsigned role,
                                     unsigned pitch);

/* Visit standing bodies in placement order; each contact trims the remaining
 * shot segment. Return the bit set of contacted roles and the last clipped
 * world point. A miss preserves hit. */
unsigned bfm_forest_bridge_targets_contact(const BfmForestBridgeTargets *targets,
                                           unsigned eligible, const int16_t from[3],
                                           const int16_t to[3], int16_t hit[3]);
unsigned bfm_forest_bridge_targets_impact(const BfmForestBridgeTargets *targets,
                                          unsigned eligible, const int16_t from[3],
                                          const int16_t to[3], int16_t hit[3], int16_t normal[3]);
#endif
