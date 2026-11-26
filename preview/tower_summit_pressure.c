/* SPDX-License-Identifier: MIT */
#include "tower_summit_pressure.h"
#include <string.h>

enum { PLATE_RADIUS = 56, SWORD_RADIUS = 48, SETTLE_UPDATES = 8 };

static int in_reach(const int16_t from[3], const int16_t target[3], int radius)
{
    int distance_squared = 0;
    for (unsigned i = 0; i < 3; ++i) {
        int delta = from[i] - target[i];
        if (delta < -radius || delta > radius) return 0;
        distance_squared += delta * delta;
    }
    return distance_squared <= radius * radius;
}

void bfm_tower_summit_pressure_init(BfmTowerSummitPressure *pressure,
                                    const int16_t plate[3], const int16_t sword[3])
{
    *pressure = (BfmTowerSummitPressure){0};
    memcpy(pressure->plate, plate, sizeof pressure->plate);
    memcpy(pressure->sword, sword, sizeof pressure->sword);
}

unsigned bfm_tower_summit_pressure_step(BfmTowerSummitPressure *pressure,
                                        const int16_t player[3],
                                        const BfmTowerSummitResidents *residents)
{
    unsigned occupied = in_reach(player, pressure->plate, PLATE_RADIUS);
    if (occupied) pressure->player_touched = 1;
    for (unsigned i = 0; !occupied && i < BFM_SUMMIT_RESIDENTS; ++i) {
        BfmTowerSummitResident resident;
        if (bfm_tower_summit_residents_get(residents, i, &resident) && resident.state != 4 && resident.state != 6)
            occupied = in_reach(resident.position, pressure->plate, PLATE_RADIUS);
    }
    if (occupied != pressure->occupied) {
        pressure->occupied = occupied;
        pressure->settle = SETTLE_UPDATES;
    }

    unsigned events = pressure->player_touched && !pressure->lesson_started
                    ? BFM_SUMMIT_FIRST_PLATE_LESSON : 0;
    if (pressure->settle) {
        --pressure->settle;
    } else if (occupied && !pressure->sword_requested && in_reach(player, pressure->sword, SWORD_RADIUS)) {
        events |= BFM_SUMMIT_SWORD_IN_REACH;
    }
    return events;
}
