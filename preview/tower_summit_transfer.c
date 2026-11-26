/* SPDX-License-Identifier: MIT */
#include "tower_summit_transfer.h"
#include <limits.h>

enum {
    BELL_INTERIOR = 0x300a, SWORD_SUMMIT = 0x300b,
    LIFT_X = 478, LIFT_Y = -1539, LIFT_Z = 173,
    WALK_START = 3, LIFT_FACING = 2844, LEAVE_LIFT = 109,
    LOAD_MODE = 117, PLACE_PLAYER = 128, REVEAL_SUMMIT = 162,
    CONTROL_READY = 17, FULL_LIGHT = 19
};

int bfm_tower_summit_transfer_begin(BfmTowerSummitTransfer *t,
                                    const RoomTransferDescriptor *destination)
{
    if (!t || t->phase != BFM_SUMMIT_TRANSFER_IDLE ||
        !destination || destination->room != SWORD_SUMMIT)
        return 0;
    t->destination = *destination;
    t->age = t->phase_age = 0;
    t->phase = BFM_SUMMIT_TRANSFER_APPROACH;
    return 1;
}

int bfm_tower_summit_transfer_at_lift(int x, int y, int z)
{
    if (x <= LIFT_X - 16 || x >= LIFT_X + 16 ||
        y <= LIFT_Y - 16 || y >= LIFT_Y + 16 ||
        z <= LIFT_Z - 16 || z >= LIFT_Z + 16)
        return 0;
    int dx = x - LIFT_X, dy = y - LIFT_Y, dz = z - LIFT_Z;
    return dx * dx + dy * dy + dz * dz < 16 * 16;
}

static void enter_phase(BfmTowerSummitTransfer *t, BfmTowerSummitPhase phase)
{
    t->phase = phase;
    t->phase_age = 0;
}

void bfm_tower_summit_transfer_reach_lift(BfmTowerSummitTransfer *t, int actor_facing)
{
    if (t && t->phase == BFM_SUMMIT_TRANSFER_APPROACH && t->age >= WALK_START) {
        t->actor_facing = (unsigned)actor_facing & 4095;
        enter_phase(t, BFM_SUMMIT_TRANSFER_ALIGN);
    }
}

int bfm_tower_step_facing(int facing, int target)
{
    int delta = target - facing;
    if (delta < -2048) delta += 4096;
    if (delta > 2048) delta -= 4096;
    int step = delta / 4;
    if (!step) step = delta / 2;
    if (!step) step = delta;
    return (unsigned)(facing + step) & 4095;
}

static void turn_on_lift(BfmTowerSummitTransfer *t)
{
    if (t->actor_facing == LIFT_FACING) enter_phase(t, BFM_SUMMIT_TRANSFER_LIFT);
    else t->actor_facing = bfm_tower_step_facing(t->actor_facing, LIFT_FACING);
}

static unsigned reveal_shade(unsigned tick)
{
    if (tick < 4) return 255;
    if (tick >= FULL_LIGHT) return 0;
    return (FULL_LIGHT - tick) * 16 - 4;
}

BfmTowerSummitFrame bfm_tower_summit_transfer_frame(const BfmTowerSummitTransfer *t)
{
    BfmTowerSummitFrame f = {0};
    f.source_room = f.room = BELL_INTERIOR;
    f.target_room = SWORD_SUMMIT;
    f.mode = 9;
    f.live = f.show_hud = f.hero_visible = 1;
    if (!t || t->phase == BFM_SUMMIT_TRANSFER_IDLE)
        return f;

    unsigned n = t->phase_age;
    f.phase = t->phase;
    f.age = t->age;
    f.target_room = t->destination.room;
    f.goal_x = LIFT_X;
    f.goal_y = LIFT_Y;
    f.goal_z = LIFT_Z;
    f.lift_facing = LIFT_FACING;
    f.actor_facing = t->actor_facing;
    f.live = f.age == 0;
    f.show_hud = f.age < 8;
    f.mode = 10;
    switch (f.phase) {
    case BFM_SUMMIT_TRANSFER_APPROACH:
        f.outgoing_walk = n >= WALK_START;
        f.outgoing_hold = n > 0 && !f.outgoing_walk;
        break;
    case BFM_SUMMIT_TRANSFER_ALIGN:
        f.outgoing_hold = 1;
        break;
    case BFM_SUMMIT_TRANSFER_LIFT:
        f.outgoing_hold = 1;
        f.wide_camera = 1;
        f.beam_age = n;
        f.hero_visible = n < 79;
        break;
    case BFM_SUMMIT_TRANSFER_LOAD:
        /* Keep drawing the interior through the fade after changing room ID. */
        f.room = t->destination.room;
        f.mode = n < LOAD_MODE ? 5 : 7;
        f.select_destination = n == 0;
        f.place_destination = n == PLACE_PLAYER;
        f.outgoing_hold = n < PLACE_PLAYER;
        f.wide_camera = 1;
        f.beam_age = LEAVE_LIFT + n;
        f.hero_visible = 0;
        if (n >= 88) f.shade = 255;
        else if (n >= 24) f.shade = (n - 24) * 4 + 2;
        break;
    case BFM_SUMMIT_TRANSFER_REVEAL:
    case BFM_SUMMIT_TRANSFER_ARRIVAL:
        f.room = t->destination.room;
        f.entrance_ready = f.phase == BFM_SUMMIT_TRANSFER_ARRIVAL;
        f.mode = f.entrance_ready ? 9 : 8;
        f.live = f.entrance_ready;
        f.show_destination = 1;
        f.hero_visible = 0;
        f.shade = reveal_shade(f.entrance_ready ? CONTROL_READY + n : n);
        break;
    default:
        break;
    }
    return f;
}

void bfm_tower_summit_transfer_advance(BfmTowerSummitTransfer *t, int destination_ready)
{
    if (!t || t->phase == BFM_SUMMIT_TRANSFER_IDLE ||
        (t->phase == BFM_SUMMIT_TRANSFER_ARRIVAL &&
         t->phase_age >= FULL_LIGHT - CONTROL_READY))
        return;
    if (t->age < UINT_MAX) ++t->age;
    if (t->phase == BFM_SUMMIT_TRANSFER_LOAD && t->phase_age == PLACE_PLAYER - 1 &&
        !destination_ready)
        return;
    if (t->phase_age < UINT_MAX) ++t->phase_age;
    if (t->phase == BFM_SUMMIT_TRANSFER_ALIGN)
        turn_on_lift(t);
    else if (t->phase == BFM_SUMMIT_TRANSFER_LIFT && t->phase_age == LEAVE_LIFT)
        enter_phase(t, BFM_SUMMIT_TRANSFER_LOAD);
    else if (t->phase == BFM_SUMMIT_TRANSFER_LOAD && t->phase_age == REVEAL_SUMMIT)
        enter_phase(t, BFM_SUMMIT_TRANSFER_REVEAL);
    else if (t->phase == BFM_SUMMIT_TRANSFER_REVEAL && t->phase_age == CONTROL_READY)
        enter_phase(t, BFM_SUMMIT_TRANSFER_ARRIVAL);
}
