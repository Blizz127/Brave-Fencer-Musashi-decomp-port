/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_BRIDGE_RETREAT_H
#define BFM_FOREST_BRIDGE_RETREAT_H

#include <stdint.h>

typedef enum {
    BFM_RETREAT_IDLE,
    BFM_RETREAT_WAIT_FOR_CONTROL,
    BFM_RETREAT_WALK,
    BFM_RETREAT_JUMP,
    BFM_RETREAT_FACE_LOGS,
    BFM_RETREAT_RELEASE,
    BFM_RETREAT_FINISHED
} BfmForestRetreatPhase;

typedef struct {
    BfmForestRetreatPhase phase;
    unsigned ticks;
} BfmForestBridgeRetreat;

typedef struct {
    int x, y, cross_pressed, cross_held;
} BfmForestRetreatPad;

/* Arm the room-local, zero-initialized controller when the room monitor
 * consumes a newly falling log and takes the tutorial from 2 to 3.
 * The earlier projectile contact and the log's first fall still belong
 * to ordinary gameplay. Start the controls on the update after acquisition. */
void bfm_forest_bridge_retreat_contact(BfmForestBridgeRetreat *retreat,
                                      unsigned tutorial, int first_log_falling);

/* Once per player update, before normal jump and locomotion processing.
 * A true return replaces the entire physical pad with the supplied pad.
 * Pass the current eye-to-target direction in the horizontal plane.
 * An airborne jump excludes its grounded landing/recovery animation.
 * The camera, animation, floor contact and movement remain engine-owned. */
int bfm_forest_bridge_retreat_controls(BfmForestBridgeRetreat *retreat,
    unsigned player_action, int airborne, int view_dx, int view_dz, int16_t *yaw,
    BfmForestRetreatPad *pad);

#endif
