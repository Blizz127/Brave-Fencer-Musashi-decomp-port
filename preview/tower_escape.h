/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_ESCAPE_H
#define BFM_TOWER_ESCAPE_H

#include "forest_entry.h"
#include "tower_escape_head.h"

typedef enum {
    BFM_ESCAPE_IDLE, BFM_ESCAPE_RETREAT, BFM_ESCAPE_WAKE, BFM_ESCAPE_BREAK,
    BFM_ESCAPE_FALL, BFM_ESCAPE_REVEAL, BFM_ESCAPE_UPPER, BFM_ESCAPE_DROP,
    BFM_ESCAPE_LAND, BFM_ESCAPE_RECOVER, BFM_ESCAPE_LOWER,
    BFM_ESCAPE_CAUGHT, BFM_ESCAPE_REQUEST, BFM_ESCAPE_TRANSFERRED
} BfmTowerEscapePhase;

typedef struct {
    BfmTowerEscapePhase phase;
    RoomTransferDescriptor destination;
    int16_t marks[4][3], lane[3];
    const uint8_t *placements, *rock_bounds, *trunk_bounds;
    int32_t x, y, z, vertical_speed, forward_speed;
    int16_t position[3], yaw;
    unsigned age, action, airborne;
    int landing, upper_lip;
    BfmTowerEscapeHead head;
} BfmTowerEscape;

/* The owner calls begin on the first update of the acquired sword's follow-up.
 * The field owns all borrowed placement and volume records for this visit. */
int bfm_tower_escape_begin(BfmTowerEscape *escape, const BfmForestField *field,
                           const int16_t position[3], int16_t yaw);
int bfm_tower_escape_tick(BfmTowerEscape *escape, const BfmForestField *field,
                          int horizontal, int cross_edge, int cross_held);
int bfm_tower_escape_take_castle(BfmTowerEscape *escape, RoomTransferDescriptor *destination);

#endif
