/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_BELL_IMPACT_H
#define BFM_TOWER_BELL_IMPACT_H

#include "forest_dust.h"

typedef struct {
    uint16_t page, clut;
    uint8_t u, v, width, height;
    int16_t left, top;
} BfmBellImage;

typedef struct {
    BfmBellImage flame[12], ring[6];
} BfmTowerBellImpact;

/* Copy the two animations' descriptors from the mounted US interior scene. */
int bfm_tower_bell_impact_load(const uint8_t *scene, size_t bytes, BfmTowerBellImpact *out);

/* Phase zero precedes the shot. Count phase_age from the director's phase
 * edge. The renderer supplies the observed display delay before the new
 * card is visible. room_age counts all actor updates since construction. */
int bfm_tower_bell_impact_card(const BfmTowerBellImpact *art, unsigned room_age,
                               unsigned room_phase, unsigned phase_age,
                               const BfmForestCamera *camera, BfmForestDustCard *out);
#endif
