/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_SHOT_IMPACT_H
#define BFM_FOREST_SHOT_IMPACT_H
#include <stddef.h>
#include <stdint.h>

/* One controller per resolved actor contact. A surface impact and an
 * expiring shot have different field effects. */
typedef struct {
    int16_t point[3], angles[3], offset[3];
    unsigned remaining;
} BfmForestShotImpact;

typedef struct {
    unsigned kind;
    int variant;
    int16_t position[3], angles[3];
} BfmForestShotEmission;

enum { BFM_FOREST_SHOT_EMISSIONS = 4 };

/* Initialize from the actual contact and outward Q12 normal. Return the
 * ordered births for this update. The secondary flash follows the first three
 * sparks, at the third spark's location. Each caller owns its output array. */
size_t bfm_forest_shot_impact_begin(BfmForestShotImpact *impact, const int16_t contact[3],
    const int16_t normal[3], BfmForestShotEmission out[BFM_FOREST_SHOT_EMISSIONS]);

/* Advance before beginning newly observed contacts in the same update. */
size_t bfm_forest_shot_impact_tick(BfmForestShotImpact *impact,
    BfmForestShotEmission out[BFM_FOREST_SHOT_EMISSIONS]);
#endif
