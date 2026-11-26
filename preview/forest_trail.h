/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_TRAIL_H
#define BFM_FOREST_TRAIL_H
#include "forest_shore.h"

typedef struct BfmForestTrail BfmForestTrail;
/* Field and shore assets outlive this first-trail encounter. */
BfmForestTrail *bfm_forest_trail_open(const BfmForestField *field, const BfmForestShore *shore);
void bfm_forest_trail_free(BfmForestTrail *trail);
/* One ordinary field-actor update, using the committed position of Musashi. */
typedef enum { BFM_TRAIL_INVALID, BFM_TRAIL_UPDATED, BFM_TRAIL_GUARD_BEYOND_RUN } BfmForestTrailUpdate;
BfmForestTrailUpdate bfm_forest_trail_step(BfmForestTrail *trail, const int16_t player[3]);
const BfmForestActor *bfm_forest_trail_trunk(const BfmForestTrail *trail);
const BfmForestActor *bfm_forest_trail_guard(const BfmForestTrail *trail);
#endif
