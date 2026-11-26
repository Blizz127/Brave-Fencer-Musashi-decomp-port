/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_SPARKS_H
#define BFM_FOREST_SPARKS_H
#include "forest_dust.h"
#include "gameplay_run.h"

typedef struct {
    int32_t position[3], submitted[3];
    int16_t velocity[3], billboard[3][2];
    unsigned age;
    int alive;
} BfmForestSpark;

typedef struct {
    BfmForestSpark sparks[BFM_FOREST_STRIKE_SPARKS];
    int32_t origin[3];
    int16_t face[3][3];
    unsigned remaining;
} BfmForestSparks;

void bfm_forest_spark_start(BfmForestSpark *spark, const int32_t origin[3],
                            const int16_t face[3][3], BfmGameplayRun *run, const int32_t eye[3]);
void bfm_forest_spark_advance(BfmForestSpark *spark);
unsigned bfm_forest_spark_project(const BfmForestSpark *spark, unsigned index,
                                  const BfmForestCamera *camera, BfmForestDustCard out[2]);

/* The displaced strike origin and orientation of the surface before strip tilt. */
void bfm_forest_sparks_start(BfmForestSparks *sparks, const int32_t origin[3],
                             const BfmForestTransform *face);
/* Apply once per world update, including the impact. */
void bfm_forest_sparks_tick(BfmForestSparks *sparks, BfmGameplayRun *run, const int32_t eye[3]);
unsigned bfm_forest_sparks_project(const BfmForestSparks *sparks, const BfmForestCamera *camera,
                                   BfmForestDustCard out[BFM_FOREST_SPARK_CARDS]);
#endif
