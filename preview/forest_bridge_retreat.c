/* SPDX-License-Identifier: MIT */
#include "forest_bridge_retreat.h"

#include <math.h>

void bfm_forest_bridge_retreat_contact(BfmForestBridgeRetreat *retreat,
                                      unsigned tutorial, int first_log_falling)
{
    if (retreat->phase == BFM_RETREAT_IDLE && tutorial == 2 && first_log_falling) {
        retreat->phase = BFM_RETREAT_WAIT_FOR_CONTROL;
        retreat->ticks = 2;
    }
}

/* Pick the digital direction closest to the actor's existing world heading
 * in this view. Screen Y is positive toward the viewer. */
static void walk_forward(int view_dx, int view_dz, int yaw, BfmForestRetreatPad *pad)
{
    static const int axes[8][2] = {
        {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}
    };
    double bearing = atan2(view_dx, view_dz) * (2048.0 / acos(-1.0));
    unsigned sector = (unsigned)(int)floor((bearing - yaw + 256.0) / 512.0) & 7;
    pad->x = axes[sector][0];
    pad->y = axes[sector][1];
}

static void jump_pad(BfmForestRetreatPad *pad)
{
    pad->x = -1;
    pad->y = 1;
    pad->cross_held = 1;
}

int bfm_forest_bridge_retreat_controls(BfmForestBridgeRetreat *retreat,
    unsigned player_action, int airborne, int view_dx, int view_dz, int16_t *yaw,
    BfmForestRetreatPad *pad)
{
    *pad = (BfmForestRetreatPad){0};
    switch (retreat->phase) {
    case BFM_RETREAT_WAIT_FOR_CONTROL:
        /* These two player actions retain priority even when the bridge
         * has already advanced beyond the retreat window. */
        if (player_action == 25 || player_action == 26) {
            if (retreat->ticks < 30) ++retreat->ticks;
            return 1;
        }
        if (retreat->ticks >= 30) {
            retreat->phase = BFM_RETREAT_RELEASE;
        } else if (player_action <= 1 && !airborne) {
            *yaw = 0;
            retreat->ticks = 0;
            retreat->phase = BFM_RETREAT_WALK;
        } else {
            ++retreat->ticks;
        }
        return 1;
    case BFM_RETREAT_WALK:
        if (retreat->ticks < 8) {
            walk_forward(view_dx, view_dz, *yaw, pad);
            ++retreat->ticks;
        } else {
            jump_pad(pad);
            pad->cross_pressed = 1;
            retreat->phase = BFM_RETREAT_JUMP;
        }
        return 1;
    case BFM_RETREAT_JUMP:
        if (airborne)
            jump_pad(pad);
        else
            retreat->phase = BFM_RETREAT_FACE_LOGS;
        return 1;
    case BFM_RETREAT_FACE_LOGS: {
        int distance = 2048 - *yaw;
        if (!distance) {
            retreat->phase = BFM_RETREAT_RELEASE;
        } else {
            int change = distance / 4;
            if (!change) change = distance / 2;
            if (!change) change = distance;
            *yaw = (int16_t)(*yaw + change);
        }
        return 1;
    }
    case BFM_RETREAT_RELEASE:
        retreat->phase = BFM_RETREAT_FINISHED;
        return 0;
    default:
        return 0;
    }
}
