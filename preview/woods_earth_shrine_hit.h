/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_EARTH_SHRINE_HIT_H
#define BFM_WOODS_EARTH_SHRINE_HIT_H

#include "forest_guard_contact.h"
#include "woods_caster_melee.h"
#include "village_ground.h"

typedef struct {
    BfmForestGuardBox bounds;
    BfmForestTransform root;
    uint8_t classes[8];
    int armed;
} BfmWoodsEarthShrineHit;

typedef enum {
    BFM_EARTH_SHRINE_MISS,
    BFM_EARTH_SHRINE_DEFLECTED,
    BFM_EARTH_SHRINE_ACCEPTED
} BfmEarthShrineHitResult;

/* Mount the fixed altar's child collider only when the room spawns the altar.
 * Failure preserves out. A zero-initialized receiver is inactive. */
int bfm_woods_earth_shrine_hit_mount(const uint8_t *scene, size_t bytes,
                                    BfmWoodsEarthShrineHit *out);

/* Pass one completed Twinpeak melee frame per field update. Only the active
 * weapon rays can contact the shrine through the mounted terrain. A deflection requests sound 1062 once
 * for this update. An accepted hit disarms the child and begins the ceremony. */
BfmEarthShrineHitResult bfm_woods_earth_shrine_hit_step(BfmWoodsEarthShrineHit *receiver,
                                                       const BfmVillageGround *ground,
                                                       const BfmWoodsCasterMeleeFrame *frame);

#endif
