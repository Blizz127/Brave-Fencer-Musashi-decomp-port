/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_ESCAPE_PROPS_H
#define BFM_TOWER_ESCAPE_PROPS_H

#include "forest_field.h"

typedef struct BfmTowerEscapeProps BfmTowerEscapeProps;
enum { BFM_TOWER_ESCAPE_PROPS = 15 };

/* Open when entering the escape road. Tick once per world update with the
 * rolling head's signed world Z before submitting the road actors. */
BfmTowerEscapeProps *bfm_tower_escape_props_open(const BfmForestField *field);
void bfm_tower_escape_props_free(BfmTowerEscapeProps *props);
void bfm_tower_escape_props_tick(BfmTowerEscapeProps *props, int16_t head_z);
size_t bfm_tower_escape_props_actors(const BfmTowerEscapeProps *props,
                                     BfmForestActor *out, size_t capacity);

#endif
