/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_SHORE_H
#define BFM_FOREST_SHORE_H
#include "forest_field.h"
#include "forest_marker.h"

typedef struct BfmForestShore BfmForestShore;
/* The field owns the mounted scene and outlives the actor. */
BfmForestShore *bfm_forest_shore_open(const BfmForestField *field,
    const uint8_t *scene, size_t bytes, uint32_t origin, const uint8_t *descriptor);
void bfm_forest_shore_free(BfmForestShore *shore);
/* Outside the supported bank, the field can continue independently. */
typedef enum { BFM_SHORE_INVALID, BFM_SHORE_UPDATED, BFM_SHORE_UNRESOLVED } BfmForestShoreUpdate;
BfmForestShoreUpdate bfm_forest_shore_step(BfmForestShore *shore, const BfmForestField *field,
                          const int16_t player[3]);
/* Active notice on this placed guard, in the same update as its model. */
const BfmForestMarker *bfm_forest_shore_marker(const BfmForestShore *shore);
const BfmForestActor *bfm_forest_shore_actor(const BfmForestShore *shore);
/* The two distinct soldier roles use the same mounted model. */
const MusashiForestPlayerMesh *bfm_forest_shore_guard_mesh(const BfmForestShore *shore);
#endif
