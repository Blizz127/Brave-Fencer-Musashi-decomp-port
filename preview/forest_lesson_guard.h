/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_LESSON_GUARD_H
#define BFM_FOREST_LESSON_GUARD_H
#include "forest_guard_contact.h"

typedef struct BfmForestLessonGuard BfmForestLessonGuard;

BfmForestLessonGuard *bfm_forest_field_lesson_guard(const BfmForestField *field);
int bfm_forest_field_summon_gun_guard(BfmForestField *field);
void bfm_forest_field_remove_gun_guard(BfmForestField *field);

/* Spawn the tutorial's authored soldier only after the scene requests it. */
BfmForestLessonGuard *bfm_forest_lesson_guard_spawn(const BfmForestField *field);
void bfm_forest_lesson_guard_free(BfmForestLessonGuard *guard);
void bfm_forest_lesson_guard_tether(BfmForestLessonGuard *guard, int held, unsigned blade_yaw);
int bfm_forest_lesson_guard_tick(BfmForestLessonGuard *guard);
const BfmForestActor *bfm_forest_lesson_guard_actor(const BfmForestLessonGuard *guard);
int bfm_forest_lesson_guard_contact(const BfmForestLessonGuard *guard,
                                    const int16_t from[3], const int16_t to[3]);
#endif
