/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_GATEWAY_TRANSITION_H
#define BFM_FOREST_GATEWAY_TRANSITION_H

#include "room_descriptor.h"
#include <stdint.h>

typedef struct {
    RoomTransferDescriptor destination;
    unsigned update;
    int active;
} BfmForestGatewayTransition;

typedef enum {
    BFM_GATEWAY_FOREST, BFM_GATEWAY_HELD_FOREST, BFM_GATEWAY_BLACK,
    BFM_GATEWAY_TOWER
} BfmForestGatewayView;

typedef enum {
    BFM_GATEWAY_KEEP, BFM_GATEWAY_STOP, BFM_GATEWAY_WALK_START,
    BFM_GATEWAY_WALK_STEP
} BfmForestGatewayMotion;

typedef struct {
    BfmForestGatewayView view;
    BfmForestGatewayMotion motion;
    uint16_t room;
    unsigned mode, live;
    unsigned select_destination, place_player; /* One update each. */
    unsigned complete; /* Normal input can resume; finish the reveal. */
    unsigned running, pose_index;
    int32_t forward_speed_q16;
    unsigned shade; /* Subtractive darkness, in 5-bit color levels. */
} BfmForestGatewayFrame;

/* Call only after the mounted forest door selects this destination. Begin
 * on the same simulation update as the north-plane crossing. */
int bfm_forest_gateway_transition_begin(BfmForestGatewayTransition *transition,
                                        const RoomTransferDescriptor *destination);
BfmForestGatewayFrame bfm_forest_gateway_transition_frame(const BfmForestGatewayTransition *transition);
/* Advance once per field simulation update (two vblanks). The loader may
 * wait for mounted destination assets before their first installation. */
void bfm_forest_gateway_transition_advance(BfmForestGatewayTransition *transition,
                                           int destination_ready);
void bfm_forest_gateway_transition_shade(const BfmForestGatewayFrame *frame,
                                         uint32_t pixels[240][320]);

#endif
