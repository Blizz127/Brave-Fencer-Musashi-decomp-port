/* SPDX-License-Identifier: MIT */
#ifndef BFM_VALLEY_ROCK_SUPPORT_H
#define BFM_VALLEY_ROCK_SUPPORT_H

#include "forest_guard_contact.h"

typedef struct {
    BfmForestGuardBox box[2];
} BfmValleyRockSupport;

typedef enum {
    BFM_VALLEY_ROCK_LANDING,
    BFM_VALLEY_ROCK_PLANTED,
    BFM_VALLEY_ROCK_SHADOW
} BfmValleyRockProbe;

int bfm_valley_rock_support_load(BfmValleyRockSupport *support,
                                 const uint8_t *scene, size_t bytes);

/* Query one of the five normal rocks at its completed actor transform.
 * A foot contact supplies Y; X and Z retain the movement's proposed stride.
 * A shadow returns the visible top, without the player's standing clearance. */
int bfm_valley_rock_support_probe(const BfmValleyRockSupport *support, unsigned model,
                                  const BfmForestTransform *root, BfmValleyRockProbe kind,
                                  const int16_t from[3], const int16_t to[3],
                                  BfmForestContact *surface);

#endif
