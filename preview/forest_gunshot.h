/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_GUNSHOT_H
#define BFM_FOREST_GUNSHOT_H

#include "forest_award.h"
#include "forest_opening_data.h"

typedef enum {
    BFM_GUNSHOT_IDLE, BFM_GUNSHOT_RAISE, BFM_GUNSHOT_RECOIL,
    BFM_GUNSHOT_LOWER, BFM_GUNSHOT_EMPTY
} BfmForestGunshotPhase;

typedef enum {
    BFM_GUNSHOT_GROUND, BFM_GUNSHOT_POSE, BFM_GUNSHOT_FIRED
} BfmForestGunshotEvent;

typedef struct {
    uint32_t pose[16];
    unsigned count, shot_key;
} BfmGunshotClip;

typedef struct {
    BfmGunshotClip raise, recoil, lower, empty;
    BfmForestGunshotPhase phase;
    unsigned key, cost;
    uint32_t pose;
} BfmForestGunshot;

/* Prepare from the completed absorption's resident resource and the mounted
 * forest. The empty-BP gesture belongs to the regular player archive. */
int bfm_forest_gunshot_load(BfmForestGunshot *gun, const BfmForestField *field,
    const BfmForestAward *award, uint32_t animation_table);

/* An ordinary Circle edge starts the grounded action. Holding Circle repeats
 * the mounted recoil clip while BP remains. Each FIRED event pays for one
 * projectile. Advance older projectiles, then launch from this pose's root.
 * Preserve the standing Fusion grip and hide Lumina (parts 24/25). */
BfmForestGunshotEvent bfm_forest_gunshot_tick(BfmForestGunshot *gun,
    BfmForestStatus *status, int circle_pressed, int circle_held, int interrupted);

/* Recoil permits aim changes without a stride. The digital pad turns toward
 * its camera-relative heading; the original analog path points immediately. */
int16_t bfm_forest_gunshot_aim(const BfmForestGunshot *gun, const BfmForestCamera *camera,
    int screen_x, int screen_y, int analog, int16_t yaw);
#endif
