/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_BRIDGE_FLOOR_H
#define BFM_FOREST_BRIDGE_FLOOR_H

#include "forest_guard_contact.h"

typedef struct {
    BfmForestGuardBox box;
} BfmForestBridgeFloor;

typedef enum {
    BFM_BRIDGE_FLOOR_SEGMENT,
    BFM_BRIDGE_FLOOR_STRIDE
} BfmForestBridgeProbe;

typedef struct {
    int16_t height;
    int16_t normal[3];
} BfmForestBridgeSurface;

int bfm_forest_bridge_floor_load_at(const BfmForestField *field, uint32_t address,
                                    BfmForestBridgeFloor *floor);
int bfm_forest_bridge_floor_load(const BfmForestField *field, BfmForestBridgeFloor *floor);

/* Query one live variant-four log at its current model transform. A segment
 * tests the supplied downward ray or landing motion. A stride adds the
 * grounded player's reach to its path. A miss preserves the output. */
int bfm_forest_bridge_floor_contact(const BfmForestBridgeFloor *floor,
    const BfmForestTransform *root, unsigned state, uint16_t flags, BfmForestBridgeProbe probe,
    const int16_t from[3], const int16_t to[3], BfmForestBridgeSurface *surface);

#endif
