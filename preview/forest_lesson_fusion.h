/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_LESSON_FUSION_H
#define BFM_FOREST_LESSON_FUSION_H
#include "forest_award.h"
#include "forest_lesson.h"
#include "forest_flight.h"
#include "gameplay_run.h"

typedef struct {
    BfmForestCast cast;
    BfmForestAward award;
    BfmFusionFlight flight;
} BfmForestLessonFusion;

/* Emit the detached blade and the trails driven by a live target. */
int bfm_forest_field_fusion_effects(BfmForestField *field, const MusashiDiscMedia *disc,
    const BfmFusionFlight *flight, const BfmForestTether *tether, int was_attached,
    int absorption_edge, const int16_t player[3], BfmGameplayRun *run);
int bfm_forest_lesson_blade_away(const BfmForestLessonFusion *fusion);
int bfm_forest_lesson_flight(BfmForestLessonFusion *fusion, BfmForestLesson *lesson,
    BfmForestField *field, const BfmForestTransform *root, const int16_t hero[3],
    int16_t yaw, unsigned tick, int square_edge, int launch);
#endif
