/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_SUMMIT_PRESSURE_H
#define BFM_TOWER_SUMMIT_PRESSURE_H

#include "tower_summit_residents.h"

typedef struct {
    int16_t plate[3], sword[3];
    unsigned occupied, settle, player_touched;
    unsigned lesson_started, sword_requested;
} BfmTowerSummitPressure;

enum {
    BFM_SUMMIT_FIRST_PLATE_LESSON = 1u << 0,
    BFM_SUMMIT_SWORD_IN_REACH = 1u << 1
};

/* Targets are taken from the registered plate and sword in the mounted room. */
void bfm_tower_summit_pressure_init(BfmTowerSummitPressure *pressure,
                                    const int16_t plate[3], const int16_t sword[3]);

/* Run once per field update with the current player feet and live residents.
 * The caller marks the lesson or pickup when its script takes ownership. */
unsigned bfm_tower_summit_pressure_step(BfmTowerSummitPressure *pressure,
                                        const int16_t player[3],
                                        const BfmTowerSummitResidents *residents);

#endif
