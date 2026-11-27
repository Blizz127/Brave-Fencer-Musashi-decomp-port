/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_GROUND_H
#define BFM_VILLAGE_GROUND_H

#include "forest_field.h"

typedef struct BfmVillageGround BfmVillageGround;
typedef enum {
    BFM_VILLAGE_GROUND_BLOCKED,
    BFM_VILLAGE_GROUND_FOOTING,
    BFM_VILLAGE_GROUND_AIR
} BfmVillageGroundResult;

/* Borrows a mounted village overlay for the lifetime of the ground object. */
BfmVillageGround *bfm_village_ground_open(const uint8_t *scene, size_t bytes, uint32_t room);
void bfm_village_ground_free(BfmVillageGround *ground);

/* Update whole coordinates only. The actor owns its velocity and fractions.
 * FOOTING also returns the supporting normal and material. AIR commits the
 * swept endpoint. BLOCKED leaves the endpoint and contact untouched. */
BfmVillageGroundResult bfm_village_ground_step(const BfmVillageGround *ground,
    const int16_t from[3], int16_t to[3], BfmForestContact *floor);
/* The optional wall material is set on successful sweeps, including zero
 * when the actor stays clear of the marked doorway surfaces. */
BfmVillageGroundResult bfm_village_ground_wall_step(const BfmVillageGround *ground,
    const int16_t from[3], int16_t to[3], BfmForestContact *floor, uint16_t *wall);
BfmVillageGroundResult bfm_village_ground_air(const BfmVillageGround *ground,
    const int16_t from[3], int16_t to[3], BfmForestContact *floor);

/* Clip a blade ray against the mounted room. A hit shortens the endpoint
 * and supplies the face's Q12 normal and full material. A miss preserves all three. */
int bfm_village_ground_blade(const BfmVillageGround *ground, const int16_t from[3],
                            int16_t end[3], int16_t normal[3], uint16_t *material);

#endif
