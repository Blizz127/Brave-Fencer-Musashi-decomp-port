/* SPDX-License-Identifier: MIT */
#include "forest_lesson_fusion.h"
#include "forest_lesson_guard.h"

static int hit_guard(void *guard, const int16_t from[3], const int16_t to[3])
{
    return bfm_forest_lesson_guard_contact(guard, from, to);
}

int bfm_forest_lesson_blade_away(const BfmForestLessonFusion *fusion)
{
    return fusion->flight.phase != BFM_FUSION_IDLE && fusion->flight.phase != BFM_FUSION_DONE;
}

int bfm_forest_lesson_flight(BfmForestLessonFusion *fusion, BfmForestLesson *lesson,
    BfmForestField *field, const BfmForestTransform *root, const int16_t hero[3],
    int16_t yaw, unsigned tick, int square_edge, int launch)
{
    BfmFusionFlight *flight = &fusion->flight;
    if (launch) {
        int16_t angles[3] = {0, yaw, 0}, tilt[3] = {0};
        return bfm_fusion_launch(flight, root, angles, tilt);
    }
    if (!bfm_forest_lesson_blade_away(fusion)) return 1;
    BfmForestLessonGuard *guard = bfm_forest_field_lesson_guard(field);
    const BfmForestActor *actor = bfm_forest_lesson_guard_actor(guard);
    int16_t aim[3];
    if (actor)
        for (unsigned i = 0; i < 3; ++i) aim[i] = (int16_t)actor->root->translation[i];
    BfmFusionFlightPhase before = flight->phase;
    BfmFusionFlightPhase after = bfm_fusion_flight_tick(flight, hero, actor ? aim : NULL, hit_guard, guard);
    /* This static field has one reserved slot for the summoned rifleman.
     * A new generation identifies each successful spawn in that slot. */
    BfmForestTetherTarget target = {0, lesson->target_generation, 47, 0x0500};
    if (after == BFM_FUSION_ATTACHED) {
        if (before != BFM_FUSION_ATTACHED)
            return actor && bfm_forest_tether_begin(&lesson->tether, field, &target);
        bfm_forest_lesson_guard_tether(guard, 1, (unsigned)flight->yaw);
        BfmForestTetherPhase phase = bfm_forest_tether_tick(&lesson->tether,
            actor ? &target : NULL, tick, square_edge, 0);
        if (phase == BFM_TETHER_EARNED || phase == BFM_TETHER_LOST) {
            bfm_forest_lesson_guard_tether(guard, 0, 0);
            bfm_fusion_release_tether(flight);
            if (phase == BFM_TETHER_EARNED) bfm_forest_field_remove_gun_guard(field);
        }
    }
    return 1;
}
