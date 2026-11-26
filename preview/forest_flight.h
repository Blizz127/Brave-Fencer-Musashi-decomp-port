/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_FLIGHT_H
#define BFM_FOREST_FLIGHT_H

#include "forest_pose.h"

typedef enum {
    BFM_FUSION_IDLE, BFM_FUSION_OUTBOUND, BFM_FUSION_ATTACHED,
    BFM_FUSION_TURNING, BFM_FUSION_RETURNING, BFM_FUSION_CATCHING, BFM_FUSION_DONE
} BfmFusionFlightPhase;

typedef struct {
    BfmFusionFlightPhase phase;
    int32_t position[3], velocity[3], speed; /* signed 16.16 world units */
    int16_t tilt[3], pitch, yaw, spin;
    unsigned timer;
    BfmForestTransform model;
} BfmFusionFlight;

/* Return positive for an assimilable actor actually intersected by the
 * segment, negative for a blocking collision and zero for a clear path. */
typedef int (*BfmFusionContact)(void *context, const int16_t from[3], const int16_t to[3]);

/* Start on the release marker of the charged throw clip. Angles are the
 * current player model's pitch, yaw and roll; tilt is the player's auxiliary
 * X/Y/Z rotation. The spawn point comes from the completed player root. */
int bfm_fusion_launch(BfmFusionFlight *flight, const BfmForestTransform *player_root,
                      const int16_t angles[3], const int16_t tilt[3]);

/* Advance one simulation update. owner is the current player's world XYZ.
 * For this single-soldier encounter, aim is the live target's position when
 * its actor has the targetable flag. Pass NULL when it is ineligible. */
BfmFusionFlightPhase bfm_fusion_flight_tick(BfmFusionFlight *flight, const int16_t owner[3],
    const int16_t aim[3], BfmFusionContact contact, void *context);
void bfm_fusion_release_tether(BfmFusionFlight *flight);

#endif
