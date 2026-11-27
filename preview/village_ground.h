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
/* An airborne passage tests the unextended foot segment and reports the
 * crossed wall material for the room's registered exit actor. */
BfmVillageGroundResult bfm_village_ground_air_passage(const BfmVillageGround *ground,
    const int16_t from[3], int16_t to[3], BfmForestContact *floor, uint16_t *wall);
/* On successful sweeps, report a vertical or steep wall contact, including
 * unmarked geometry. The contact flag is independent of the wall material. */
BfmVillageGroundResult bfm_village_ground_air_contact(const BfmVillageGround *ground,
    const int16_t from[3], int16_t to[3], BfmForestContact *floor, int *wall_hit);

/* Both forest banks have marked thorn tops and sides. Sample the swept foot
 * against the actual room, returning ordinary wall/footing information and
 * latching the last marked surface for the environmental event dispatcher.
 * Keep the reason across updates until its handler consumes it. */
BfmVillageGroundResult bfm_village_ground_bank_step(const BfmVillageGround *ground,
    const int16_t from[3], int16_t to[3], int airborne, BfmForestContact *floor,
    uint16_t *wall, uint16_t *reason);

/* Bank recovery samples the mounted cell independently of an ordinary stride.
 * Mode 0 extends a walking step, 1 uses the given segment, and 2 samples six
 * units below from. A successful projection returns its endpoint and slope
 * bits (gentle 0x2000, incline 0x4000, steep or wall 0x8000). Material can
 * also identify a crossed passage on a miss. */
unsigned bfm_village_ground_support_probe(const BfmVillageGround *ground, unsigned mode,
    const int16_t from[3], int16_t to[3], uint16_t *material);

/* A gentle floor alone is suitable for a rescue position. Reject an
 * intervening wall even when an ordinary walking sweep could slide past it. */
int bfm_village_ground_clear_floor(const BfmVillageGround *ground, const int16_t from[3],
                                   int16_t to[3], int vertical_probe, BfmForestContact *floor);

/* Clip a blade ray against the mounted room. A hit shortens the endpoint
 * and supplies the face's Q12 normal and full material. A miss preserves all three. */
int bfm_village_ground_blade(const BfmVillageGround *ground, const int16_t from[3],
                            int16_t end[3], int16_t normal[3], uint16_t *material);

#endif
