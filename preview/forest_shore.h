/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_SHORE_H
#define BFM_FOREST_SHORE_H
#include "forest_field.h"
#include "forest_marker.h"

typedef struct BfmForestShore BfmForestShore;
/* The field owns the mounted scene and outlives the actor. */
BfmForestShore *bfm_forest_shore_open(const BfmForestField *field,
    const uint8_t *scene, size_t bytes, uint32_t origin, const uint8_t *descriptor);
/* Each authored melee placement owns its own patrol and combat state. */
BfmForestShore *bfm_forest_shore_open_patrol(const BfmForestShore *primary,
                                           const uint8_t placement[20]);
void bfm_forest_shore_free(BfmForestShore *shore);
/* Call before the guard update on a confirmed nonlethal first hit. */
void bfm_forest_shore_recoil(BfmForestShore *shore, uint16_t attacker_yaw);
/* Apply the incoming strength once the field confirms a distinct hit.
 * Covers nonlethal hits outside an active recoil. Other cases return zero. */
int bfm_forest_shore_take_hit(BfmForestShore *shore, uint16_t attacker_yaw, uint16_t strength);
/* Each placed target remembers the attack kind until recoil ends. */
int bfm_forest_shore_take_typed_hit(BfmForestShore *shore, uint16_t attacker_yaw,
                                    uint16_t strength, uint16_t attack_type);
/* Outside the supported bank, the field can continue independently. */
typedef enum { BFM_SHORE_INVALID, BFM_SHORE_UPDATED, BFM_SHORE_UNRESOLVED } BfmForestShoreUpdate;
BfmForestShoreUpdate bfm_forest_shore_step(BfmForestShore *shore, const BfmForestField *field,
                          const int16_t player[3], const int16_t target[3], BfmGameplayRun *run);
/* Active notice on this placed guard, in the same update as its model. */
const BfmForestTransform *bfm_forest_shore_strike(const BfmForestShore *shore, int16_t *yaw);
const BfmForestMarker *bfm_forest_shore_marker(const BfmForestShore *shore);
const BfmForestActor *bfm_forest_shore_actor(const BfmForestShore *shore);
/* The two distinct soldier roles use the same mounted model. */
const MusashiForestPlayerMesh *bfm_forest_shore_guard_mesh(const BfmForestShore *shore);
#endif
