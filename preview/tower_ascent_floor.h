/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_ASCENT_FLOOR_H
#define BFM_TOWER_ASCENT_FLOOR_H

#include "forest_field.h"

/* Install the raised courtyard rendering and collision meshes together.
 * Repeated activation is harmless. A failed mount retains the submerged pair. */
int bfm_tower_ascent_floor_activate(BfmForestField *field);

/* BLOCKED rejects a wall or a bad starting surface. FREE means that this
 * stride lost its supporting tread. Only GROUNDED replaces the endpoint. */
BfmForestTopStep bfm_tower_ascent_floor_step(const BfmForestField *field,
    const BfmForestContact *previous, const int16_t from[3], int16_t to[3]);

#endif
