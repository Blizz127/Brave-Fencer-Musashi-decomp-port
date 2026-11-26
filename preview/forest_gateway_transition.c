/* SPDX-License-Identifier: MIT */
#include "forest_gateway_transition.h"

enum {
    FOREST = 0x3008, TOWER = 0x3009,
    SELECT_ROOM = 30, OUTGOING_WIPE = 41, OUTGOING_BLACK = 125,
    LOAD_ROOM = 143, INSTALL_ENTRY = 145, BEGIN_ENTRY = 189,
    ENTER_WALK = 191, ROOM_LIVE = 206, FULL_LIGHT = 208, FINISH_WALK = ENTER_WALK + 30
};

int bfm_forest_gateway_transition_begin(BfmForestGatewayTransition *t,
                                        const RoomTransferDescriptor *destination)
{
    if (!t || t->active || !destination || destination->room != TOWER)
        return 0;
    t->destination = *destination;
    t->update = 0;
    t->active = 1;
    return 1;
}

static void walk(BfmForestGatewayFrame *frame, unsigned steps)
{
    frame->motion = steps ? BFM_GATEWAY_WALK_STEP : BFM_GATEWAY_WALK_START;
    frame->forward_speed_q16 = steps ? (int32_t)((8u << 16) + steps * 4096u) : 0;
    frame->running = steps >= 21;
    frame->pose_index = frame->running ? steps - 21 : steps % 16;
}

BfmForestGatewayFrame bfm_forest_gateway_transition_frame(const BfmForestGatewayTransition *t)
{
    BfmForestGatewayFrame frame = {0};
    frame.room = FOREST;
    frame.mode = 9;
    frame.live = 1;
    if (!t || !t->active)
        return frame;

    unsigned n = t->update;
    frame.room = n < SELECT_ROOM ? FOREST : t->destination.room;
    frame.live = n < 2 || n >= ROOM_LIVE;
    frame.select_destination = n == SELECT_ROOM;
    frame.place_player = n == INSTALL_ENTRY;
    frame.complete = n >= FINISH_WALK;

    if (n < SELECT_ROOM) {
        if (n) frame.mode = 10;
        if (n == 2) frame.motion = BFM_GATEWAY_STOP;
        if (n >= 3) walk(&frame, n - 3);
        return frame;
    }

    frame.mode = n < LOAD_ROOM ? 5 : n < BEGIN_ENTRY ? 7 : n < ROOM_LIVE ? 8 : 9;
    if (n < OUTGOING_BLACK) {
        frame.view = BFM_GATEWAY_HELD_FOREST;
        if (n > OUTGOING_WIPE)
            frame.shade = (n - OUTGOING_WIPE) / 2;
    } else if (n <= ENTER_WALK + 1) {
        frame.view = BFM_GATEWAY_BLACK;
        frame.shade = 31;
    } else {
        frame.view = BFM_GATEWAY_TOWER;
        frame.shade = n < FULL_LIGHT ? (FULL_LIGHT - n) * 2 - 1 : 0;
    }

    if (n == BEGIN_ENTRY + 1)
        frame.motion = BFM_GATEWAY_STOP;
    if (n >= ENTER_WALK && n < FINISH_WALK)
        walk(&frame, n - ENTER_WALK);
    return frame;
}

void bfm_forest_gateway_transition_advance(BfmForestGatewayTransition *t,
                                           int destination_ready)
{
    if (t && t->active && t->update < FINISH_WALK &&
        (t->update != INSTALL_ENTRY - 1 || destination_ready))
        ++t->update;
}

void bfm_forest_gateway_transition_shade(const BfmForestGatewayFrame *frame,
                                         uint32_t pixels[240][320])
{
    if (!frame || !pixels || !frame->shade) return;
    if (frame->view == BFM_GATEWAY_BLACK) {
        for (unsigned y = 0; y < 240; ++y)
            for (unsigned x = 0; x < 320; ++x)
                pixels[y][x] = 0xff000000u;
        return;
    }
    for (unsigned y = 0; y < 240; ++y) {
        unsigned edge = y < 120 ? y : 239 - y;
        unsigned delay = 0;
        if (frame->view == BFM_GATEWAY_HELD_FOREST && edge > 24)
            delay = (edge - 24) / 8;
        if (delay > 11) delay = 11;
        if (frame->shade <= delay) continue;
        unsigned darkness = frame->shade - delay;
        for (unsigned x = 0; x < 320; ++x) {
            uint32_t p = pixels[y][x], out = 0xff000000u;
            for (unsigned shift = 0; shift <= 16; shift += 8) {
                unsigned level = (p >> shift) & 255u;
                level >>= 3;
                if (level > darkness)
                    out |= ((level - darkness) * 255 / 31) << shift;
            }
            pixels[y][x] = out;
        }
    }
}
