/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_MOTION_H
#define BFM_FOREST_MOTION_H

#include "forest_player_state.h"
#include <stdint.h>

/* Convert the current local player velocities to world-space velocities at the
 * normal rotation stage. Pass the three angles owned by the current model.
 * Returns zero without touching the player outside the measured level range.
 * The matching position update runs later, after the action's speed limit. */
int bfm_forest_level_velocity(ForestPlayerState *player, int16_t pitch,
                              int16_t yaw, int16_t roll);

#endif
