/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_ESCAPE_HEAD_H
#define BFM_TOWER_ESCAPE_HEAD_H

#include "forest_field.h"

typedef struct {
    BfmForestTransform root;
    int16_t position[3], yaw, roll;
    int visible, caught, exit_ready;
    int32_t y, z, vy, vz, gravity;
    unsigned phase, motion, spin, pending_zone, age, clock;
} BfmTowerEscapeHead;

/* The visit owns the mounted stone figure. Its world transform below can
 * replace that figure's pedestal transform after the sword scene takes over. */
int bfm_tower_escape_head_begin(BfmTowerEscapeHead *head, const BfmForestField *room);
int bfm_tower_escape_head_tick(BfmTowerEscapeHead *head, const BfmForestField *room,
                               unsigned phase, unsigned age, const int16_t player[3]);

#endif
