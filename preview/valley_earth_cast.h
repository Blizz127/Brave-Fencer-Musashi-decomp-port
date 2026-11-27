/* SPDX-License-Identifier: MIT */
#ifndef BFM_VALLEY_EARTH_CAST_H
#define BFM_VALLEY_EARTH_CAST_H

#include "bedroom_record.h"
#include "field_input.h"

typedef struct {
    unsigned charging, phase, remaining;
    int height_eighths, velocity_eighths;
} BfmValleyEarthCast;

typedef struct {
    int charging, casting, quake, finished;
    int height;       /* Signed world Y offset from the stationary takeoff position. */
    uint32_t program; /* Mounted Musashi body program for this field. */
} BfmValleyEarthFrame;

/* Step once per field update. can_begin admits a controlled, grounded player.
 * Charging permits ordinary ground motion. During a cast the room stages the
 * returned body at the takeoff X/Z and height, and consumes quake once. */
BfmValleyEarthFrame bfm_valley_earth_cast_step(BfmValleyEarthCast *cast,
    BfmSaveRecord *record, BfmFieldInput input, int can_begin);

#endif
