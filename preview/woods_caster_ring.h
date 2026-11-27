/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_CASTER_RING_H
#define BFM_WOODS_CASTER_RING_H

#include "bedroom_figure.h"
#include "forest_status.h"

typedef struct BfmWoodsCasterRings BfmWoodsCasterRings;

typedef struct {
    unsigned flags, power, yaw;
    int16_t point[3], normal[3];
} BfmWoodsCasterRingHit;

BfmWoodsCasterRings *bfm_woods_rings_load(const uint8_t *scene, size_t bytes);
void bfm_woods_rings_free(BfmWoodsCasterRings *rings);
void bfm_woods_rings_emit(BfmWoodsCasterRings *rings, const int16_t feet[3],
                          unsigned yaw, unsigned stage, unsigned option);
void bfm_woods_rings_step(BfmWoodsCasterRings *rings, const int16_t feet[3],
                          int mini, unsigned option);
const BfmWoodsCasterRingHit *bfm_woods_rings_hit(const BfmWoodsCasterRings *rings);
int bfm_woods_rings_add(const BfmWoodsCasterRings *rings, BfmBedroomFigure *figure,
                        const BfmBedroomView *view);

/* The owning player applies Mini only after accepting a status-bearing hit.
 * Advance the saved clock once per active, eligible player update. */
int bfm_woods_ring_mini_active(const BfmForestStatus *status);
void bfm_woods_ring_mini_apply(BfmForestStatus *status);
void bfm_woods_ring_mini_step(BfmForestStatus *status);

#endif
