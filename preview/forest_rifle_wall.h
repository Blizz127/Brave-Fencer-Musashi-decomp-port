/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_RIFLE_WALL_H
#define BFM_FOREST_RIFLE_WALL_H

#include "forest_field.h"

/* Sweep the previous and new integer origins of a bank sentry's round,
 * before its victim test. A nonzero terrain flag or material ends the
 * round and supplies the world position for its impact effect. */
unsigned bfm_forest_rifle_wall(const BfmForestField *field, const int16_t from[3],
                               const int16_t to[3], int16_t impact[3]);

#endif
