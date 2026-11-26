/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_TRAIL_H
#define BFM_FOREST_TRAIL_H
#include "forest_shore.h"

/* Decode one rigid trunk from its mounted room model list. */
MusashiForestPlayerMesh *bfm_forest_trunk_mesh(const BfmForestField *field, uint32_t table);

typedef struct BfmForestTrail BfmForestTrail;
/* Field and shore assets outlive this first-trail encounter. */
BfmForestTrail *bfm_forest_trail_open(const BfmForestField *field, const BfmForestShore *shore);
void bfm_forest_trail_free(BfmForestTrail *trail);
/* One ordinary field-actor update, using the committed position of Musashi. */
typedef enum { BFM_TRAIL_INVALID, BFM_TRAIL_UPDATED, BFM_TRAIL_GUARD_BEYOND_RUN } BfmForestTrailUpdate;
BfmForestTrailUpdate bfm_forest_trail_step(BfmForestTrail *trail, const int16_t player[3]);
/* Remaining burst updates after the fallen trunk settles. The current
 * countdown selects which side of the player emits this update. */
unsigned bfm_forest_trail_scatter(const BfmForestTrail *trail);
const BfmForestActor *bfm_forest_trail_trunk(const BfmForestTrail *trail);
const BfmForestActor *bfm_forest_trail_guard(const BfmForestTrail *trail);
#endif
