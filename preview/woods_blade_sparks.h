/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_BLADE_SPARKS_H
#define BFM_WOODS_BLADE_SPARKS_H

#include "forest_dust.h"

/* One emission per update, visible on the birth update and ten later ones. */
enum {
    BFM_WOODS_BLADE_SPARK_TICKS = 11,
    BFM_WOODS_BLADE_SPARKS = BFM_WOODS_BLADE_SPARK_TICKS,
    BFM_WOODS_BLADE_SPARK_CARDS = 2 * BFM_WOODS_BLADE_SPARKS
};

typedef struct {
    BfmForestTransform face;
    uint32_t position_q16[3];
    int32_t velocity_q16[3];
    unsigned remaining;
} BfmWoodsBladeSpark;

typedef struct {
    BfmWoodsBladeSpark slots[BFM_WOODS_BLADE_SPARKS];
} BfmWoodsBladeSparks;

/* Advance the previous emissions before inserting this update's sparkle.
 * Surviving sparks continue to drift after a latch or catch. */
void bfm_woods_blade_sparks_tick(BfmWoodsBladeSparks *sparks);
/* center is the blade actor's integral world position; blade supplies its
 * unscaled orientation. The emitter passes one room RNG draw per birth. */
void bfm_woods_blade_sparks_emit(BfmWoodsBladeSparks *sparks, const int32_t center[3],
                                const BfmForestTransform *blade, const int32_t eye[3],
                                unsigned random);
size_t bfm_woods_blade_sparks_project(const BfmWoodsBladeSparks *sparks,
                        const BfmForestCamera *camera, BfmForestDustCard *cards, size_t capacity);

#endif
