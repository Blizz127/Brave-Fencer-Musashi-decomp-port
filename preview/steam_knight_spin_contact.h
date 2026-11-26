/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_SPIN_CONTACT_H
#define BFM_STEAM_KNIGHT_SPIN_CONTACT_H

#include "forest_pose.h"
#include "gameplay_run.h"

typedef struct {
    struct { uint16_t body, fusion, lumina, exp_limit[4]; } ranks[30];
    uint16_t fusion_percent, fusion_spread, lumina_percent, lumina_spread;
    uint16_t target_mask;
} BfmSteamSpinData;

typedef struct {
    uint16_t strength;
    int critical;
} BfmSteamSpinRoll;

typedef struct {
    int16_t pivot[3], ends[3][3];
    unsigned count;
} BfmSteamSpinSegments;

/* Mount the equipped first scroll's charged move from the current hall. */
int bfm_steam_spin_load(const MusashiDiscMedia *disc, const uint8_t *scene,
                        size_t bytes, BfmSteamSpinData *out);

/* Issue once per live charged rotation update, before the contact window
 * is tested. All targets and segments in that update share this roll. */
int bfm_steam_spin_roll(const BfmSteamSpinData *data, BfmGameplayRun *run,
                        uint32_t actor_flags,
                        BfmSteamSpinRoll *out);

/* The common actor transform includes current facing. previous_in_current
 * supplies the signed previous-minus-current yaw as a Q12 rotation. Its
 * translation is ignored. Endpoints are returned in their collision order. */
int bfm_steam_spin_segments(const BfmForestTransform *actor,
                            const BfmForestTransform *previous_in_current,
                            BfmSteamSpinSegments *out);

/* Use the consecutive model headings saved by the charged turn. */
int bfm_steam_spin_segments_yaw(const BfmForestTransform *actor, int16_t previous_yaw,
                                int16_t current_yaw, BfmSteamSpinSegments *out);

#endif
