/* SPDX-License-Identifier: MIT */
#ifndef BFM_HELLS_VALLEY_BOULDERS_H
#define BFM_HELLS_VALLEY_BOULDERS_H

#include "bedroom_figure.h"
#include "valley_rock_support.h"
#include "valley_hinge.h"
#include "village_ground.h"

typedef struct BfmHellsValleyBoulders BfmHellsValleyBoulders;

/* The valley's six rocks start on the cliffs or on their individually saved
 * landings. Their models and placements come from the mounted field. */
BfmHellsValleyBoulders *bfm_hells_valley_boulders_open(const uint8_t *scene,
    size_t bytes, const uint8_t flags[64]);
void bfm_hells_valley_boulders_free(BfmHellsValleyBoulders *rocks);
/* Each bit identifies the room placement that produced the event. */
typedef struct {
    unsigned started, shake, left_cliff, landed;
    BfmValleyHingeEvent hinge;
} BfmValleyBoulderEvents;
BfmValleyBoulderEvents bfm_hells_valley_boulders_step(BfmHellsValleyBoulders *rocks,
    const BfmBedroomView *view, const BfmVillageGround *ground, int earth_quake, const int16_t foot[3],
    uint8_t saved_flags[64], uint8_t live_flags[64]);
unsigned bfm_hells_valley_boulders_support(const BfmHellsValleyBoulders *rocks,
    BfmValleyRockProbe kind, const int16_t from[3], const int16_t to[3], BfmForestContact *surface);
int bfm_hells_valley_hinge_foot(const BfmHellsValleyBoulders *rocks, unsigned rider,
    int short_probe, const int16_t previous[3], int16_t foot[3], unsigned *attached, BfmForestContact *surface);
int bfm_hells_valley_hinge_shadow(const BfmHellsValleyBoulders *rocks, const int16_t foot[3],
    int16_t position[3], int16_t normal[3]);
int bfm_hells_valley_hinge_body(const BfmHellsValleyBoulders *rocks, unsigned rider,
    const int16_t previous[3], int16_t foot[3], unsigned field_tick, int airborne);
int bfm_hells_valley_boulders_draw(void *rocks, BfmBedroomFigure *figure,
                                   const BfmBedroomView *view);

#endif
