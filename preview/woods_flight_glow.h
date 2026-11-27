/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_FLIGHT_GLOW_H
#define BFM_WOODS_FLIGHT_GLOW_H

#include "forest_flight.h"
#include "woods_blade_sparks.h"
#include "woods_capture_light.h"

enum { BFM_WOODS_HALO_CARDS = 16 };

typedef struct BfmWoodsFlightGlow {
    const uint8_t *edge, *round, *attached_ring;
    BfmWoodsBladeSparks sparks;
    BfmWoodsCaptureLight capture;
    int pulse, threshold, delta, evolution, visible_evolution;
    unsigned meter, attached_ticks;
    int alive, gripped, draw_blade, raised;
    BfmForestTransform raised_blade;
} BfmWoodsFlightGlow;

/* Contours borrow the current mounted woods scene for the room lifetime. */
int bfm_woods_flight_glow_load(BfmWoodsFlightGlow *art, const uint8_t *scene, size_t size);
int bfm_field_flight_glow_load(BfmWoodsFlightGlow *art, const uint8_t *scene, size_t bytes,
                               uint32_t edge_address);
void bfm_woods_flight_glow_tick(BfmWoodsFlightGlow *art, const BfmFusionFlight *flight,
    unsigned meter, const BfmForestTransform *award_owner, unsigned field_tick,
    const int32_t eye[3], BfmTetherRandom random, void *context);
size_t bfm_woods_flight_glow_project(const BfmWoodsFlightGlow *art, const BfmFusionFlight *flight,
    const BfmForestCamera *camera, const int32_t eye[3], BfmForestDustCard *out, size_t capacity);

#endif
