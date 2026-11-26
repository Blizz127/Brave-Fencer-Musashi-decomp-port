/* SPDX-License-Identifier: MIT */
#include "steam_knight_corner.h"
#include "field_waypoint.h"
#include "forest_manual.h"
#include <string.h>

int bfm_steam_corner_begin(BfmSteamCorner *corner, const int16_t target[3])
{
    if (!corner || !target || corner->phase != BFM_CORNER_UNUSED) return 0;
    memcpy(corner->target, target, sizeof corner->target);
    corner->phase = BFM_CORNER_ENTRY;
    return 1;
}

static void idle(ForestPlayerState *actor)
{
    memset(actor->velocity, 0, sizeof actor->velocity);
    actor->field_24.word = actor->local_y.word = actor->field_2c.word = 0;
    actor->state = 0;
    actor->phase = 1;
}

static int arrive(const BfmSteamCorner *corner, const BfmForestField *field,
                  ForestPlayerState *actor, int16_t feet[3])
{
    BfmForestContact ground;
    int16_t to[3] = {corner->target[0], feet[1], corner->target[2]};
    if (!bfm_forest_ground(field, feet[0], feet[1], feet[2], &ground) ||
        !bfm_forest_corridor_step(field, &ground, feet, to) ||
        memcmp(to, corner->target, sizeof to)) return 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        uint32_t fraction = actor->position[axis] & 0xffffu;
        actor->position[axis] = ((uint32_t)(uint16_t)to[axis] << 16) | fraction;
    }
    memcpy(feet, to, sizeof to);
    idle(actor);
    return 1;
}

BfmSteamCornerStep bfm_steam_corner_step(BfmSteamCorner *corner, const BfmForestField *field,
    ForestPlayerState *actor, int16_t feet[3], int16_t *yaw, int camera_yaw, int west_view_settled)
{
    if (!corner || !field || !actor || !feet || !yaw) return BFM_CORNER_BLOCKED;
    switch (corner->phase) {
    case BFM_CORNER_UNUSED:
    case BFM_CORNER_DONE:
        return BFM_CORNER_CONTROLS;
    case BFM_CORNER_ENTRY:
        idle(actor);
        corner->phase = BFM_CORNER_DISPATCH;
        return BFM_CORNER_IDLE;
    case BFM_CORNER_DISPATCH:
        corner->phase = BFM_CORNER_START_WALK;
        return BFM_CORNER_IDLE;
    case BFM_CORNER_WAIT_VIEW:
        if (west_view_settled) corner->phase = BFM_CORNER_VIEW_READY;
        return BFM_CORNER_IDLE;
    case BFM_CORNER_VIEW_READY:
        corner->phase = BFM_CORNER_DONE;
        return BFM_CORNER_CONTROLS;
    case BFM_CORNER_START_WALK:
    case BFM_CORNER_WALKING:
        break;
    default:
        return BFM_CORNER_BLOCKED;
    }
    int dx = corner->target[0] - feet[0], dz = corner->target[2] - feet[2];
    if ((int64_t)dx * dx + (int64_t)dz * dz < 16 * 16) {
        if (!arrive(corner, field, actor, feet)) return BFM_CORNER_BLOCKED;
        corner->phase = BFM_CORNER_WAIT_VIEW;
        return BFM_CORNER_AT_MARK;
    }
    BfmWaypointInput input = bfm_waypoint_input(dx, dz, 0, camera_yaw & 4095);
    if (corner->phase == BFM_CORNER_START_WALK) {
        actor->state = actor->phase = 1;
        corner->phase = BFM_CORNER_WALKING;
    } else {
        ForestPlayerState next = *actor;
        next.field_2c.word = -input.speed_q16;
        if (!bfm_forest_manual_coast(field, &next, input.yaw, feet)) return BFM_CORNER_BLOCKED;
        *actor = next;
    }
    *yaw = input.yaw;
    return BFM_CORNER_WALK;
}
