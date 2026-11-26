/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_SUMMIT_TRANSFER_H
#define BFM_TOWER_SUMMIT_TRANSFER_H

#include "room_descriptor.h"
#include <stdint.h>

typedef enum {
    BFM_SUMMIT_TRANSFER_IDLE,
    BFM_SUMMIT_TRANSFER_APPROACH,
    BFM_SUMMIT_TRANSFER_ALIGN,
    BFM_SUMMIT_TRANSFER_LIFT,
    BFM_SUMMIT_TRANSFER_LOAD,
    BFM_SUMMIT_TRANSFER_REVEAL,
    BFM_SUMMIT_TRANSFER_ARRIVAL
} BfmTowerSummitPhase;

typedef struct {
    RoomTransferDescriptor destination;
    unsigned age, phase_age;
    int actor_facing;
    BfmTowerSummitPhase phase;
} BfmTowerSummitTransfer;

typedef struct {
    BfmTowerSummitPhase phase;
    unsigned age;
    uint16_t source_room, target_room, room;
    unsigned mode, live;
    unsigned select_destination, place_destination, entrance_ready;
    unsigned outgoing_walk, outgoing_hold;
    int goal_x, goal_y, goal_z, lift_facing, actor_facing;
    unsigned wide_camera, show_destination, hero_visible, show_hud;
    unsigned beam_age; /* Valid after wide_camera becomes true. */
    unsigned shade;      /* 0..255 intensity subtracted from each channel. */
} BfmTowerSummitFrame;

/* Advance one shortest-arc turn in the field's 4096-unit angle space. */
int bfm_tower_step_facing(int facing, int target);

/* Begin on the update when the interior door accepts the player. */
int bfm_tower_summit_transfer_begin(BfmTowerSummitTransfer *transfer,
                                    const RoomTransferDescriptor *destination);
BfmTowerSummitFrame bfm_tower_summit_transfer_frame(const BfmTowerSummitTransfer *transfer);
/* Test the world's integer actor coordinates before each commanded step. */
int bfm_tower_summit_transfer_at_lift(int x, int y, int z);
/* Once the walker reaches the goal, align X/Z to the board and hold. */
void bfm_tower_summit_transfer_reach_lift(BfmTowerSummitTransfer *transfer, int actor_facing);
/* One update per two video fields. A late loader waits under black. */
void bfm_tower_summit_transfer_advance(BfmTowerSummitTransfer *transfer, int destination_ready);

#endif
