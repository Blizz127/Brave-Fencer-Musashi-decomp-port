/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_SHOT_SPARK_H
#define BFM_FOREST_SHOT_SPARK_H

#include "forest_dust.h"

enum { BFM_FOREST_SHOT_SPARK_CARDS = 2 };

typedef struct {
    BfmForestTransform face;
    int16_t position[3], velocity[3];
    unsigned step, variant;
} BfmForestShotSpark;

/* Seed one child with its emitting controller's pitch/yaw/roll and the
 * scatter RNG draw for this slot. Gunshot contact uses variants 2 and 3. */
int bfm_forest_shot_spark_fire(BfmForestShotSpark *spark, const int16_t origin[3],
                  const int16_t direction_angles[3], unsigned variant,
                  unsigned random, const int32_t eye[3]);
/* Advance existing children once per field simulation update before firing
 * new ones. The spawn update has no visible card. */
void bfm_forest_shot_spark_tick(BfmForestShotSpark *spark);
size_t bfm_forest_shot_spark_project(const BfmForestShotSpark *spark,
                  const BfmForestCamera *camera, BfmForestDustCard *cards, size_t capacity);
#endif
