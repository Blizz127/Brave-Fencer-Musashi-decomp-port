/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_SHORE_THROW_H
#define BFM_FOREST_SHORE_THROW_H

#include "forest_field.h"

typedef struct {
    int32_t position[3], velocity[3]; /* Q16 world space */
    int16_t yaw, pitch, probe_height;
    unsigned phase, ticks, grounded, key, landed_key;
    uint32_t clip, tumble_clip;
} BfmForestShoreThrow;

typedef enum {
    BFM_SHORE_THROW_UNRESOLVED,
    BFM_SHORE_THROW_MOVING,
    BFM_SHORE_THROW_RECOVER
} BfmForestShoreThrowStep;

/* A normal throw uses kind 9. An interrupted lift or carry uses kind 17
 * and its own mounted velocity. Tick either on the release update. */
int bfm_forest_shore_release_begin(BfmForestShoreThrow *motion, const BfmForestField *field,
                                   const int32_t position[3], int attacker_yaw, unsigned kind);
int bfm_forest_shore_throw_begin(BfmForestShoreThrow *motion, const BfmForestField *field,
                                 const int32_t position[3], int attacker_yaw);
/* Traverse ordinary flat-bank terrain. Unsupported contacts preserve motion.
 * RECOVER owns the final movement and releases the ordinary behavior. */
BfmForestShoreThrowStep bfm_forest_shore_throw_tick(BfmForestShoreThrow *motion,
                                                    const BfmForestField *field);
#endif
