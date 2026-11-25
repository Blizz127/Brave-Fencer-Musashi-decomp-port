/* SPDX-License-Identifier: MIT */
#include "forest_manual.h"
#include "forest_action.h"
#include "forest_motion.h"
#include "forest_opening_data.h"

#include <math.h>
#include <string.h>

enum { WALK = 7, RUN = 8, Q16 = 65536, ACCELERATION = Q16 / 16 };

/* Resolve a screen vector on the level ground. The camera can pitch, so
 * normalize its two projected axes separately before combining them. */
static int ground_heading(const BfmForestCamera *camera, int x, int y, int16_t *yaw)
{
    double rx = camera->rotation[0][0], rz = camera->rotation[0][2];
    double fx = camera->rotation[2][0], fz = camera->rotation[2][2];
    double right = hypot(rx, rz), forward = hypot(fx, fz);
    if (!right || !forward)
        return 0;
    double world_x = x * rx / right - y * fx / forward;
    double world_z = x * rz / right - y * fz / forward;
    if (hypot(world_x, world_z) < 0.5)
        return 0;
    /* Local forward is negative Z in the player model. */
    double turns = atan2(-world_x, -world_z) / (2.0 * acos(-1.0));
    *yaw = (int16_t)((unsigned)lround(turns * 4096.0) & 4095);
    return 1;
}

BfmForestManualResult bfm_forest_manual_tick(BfmForestManual *state,
    const BfmForestField *field, ForestPlayerState *actor, const BfmForestCamera *camera,
    int screen_x, int screen_y, int16_t *yaw, int16_t cached[3])
{
    if (!state || !field || !actor || !camera || !yaw || !cached ||
        screen_x < -1 || screen_x > 1 || screen_y < -1 || screen_y > 1)
        return BFM_FOREST_MANUAL_ERROR;

    if (!screen_x && !screen_y) {
        int moving = state->moving;
        memset(actor->velocity, 0, sizeof actor->velocity);
        actor->field_24.word = 0;
        actor->local_y.word = 0;
        actor->field_2c.word = 0;
        actor->state = 0;
        actor->phase = 1;
        state->moving = 0;
        state->motion = state->elapsed = 0;
        return moving ? BFM_FOREST_MANUAL_STOPPED : BFM_FOREST_MANUAL_IDLE;
    }

    int16_t next_yaw;
    if (!ground_heading(camera, screen_x, screen_y, &next_yaw))
        return BFM_FOREST_MANUAL_ERROR;
    if (!state->moving) {
        BfmForestOpeningData data;
        if (!bfm_forest_opening_data(field, &data))
            return BFM_FOREST_MANUAL_ERROR;
        memset(actor->velocity, 0, sizeof actor->velocity);
        actor->field_24.word = actor->local_y.word = 0;
        actor->field_2c.word = data.initial_velocity;
        actor->state = actor->phase = 1;
        if (!state->have_origin) {
            memcpy(state->origin, cached, sizeof state->origin);
            state->have_origin = 1;
        }
        state->moving = 1;
        state->motion = WALK;
        state->elapsed = 0;
        return BFM_FOREST_MANUAL_STARTED;
    }

    ForestPlayerState next = *actor;
    int32_t speed = next.field_2c.word;
    if (speed > 0 || speed < -11 * Q16)
        return BFM_FOREST_MANUAL_ERROR;
    speed -= ACCELERATION;
    if (speed < -11 * Q16)
        speed = -11 * Q16;
    next.field_2c.word = speed;
    if (!bfm_forest_level_velocity(&next, 0, next_yaw, 0))
        return BFM_FOREST_MANUAL_ERROR;
    func_801473EC(&next);
    int16_t end[3] = {(int16_t)(next.position[0] >> 16), cached[1],
                      (int16_t)(next.position[2] >> 16)};
    BfmForestContact ground;
    if (!bfm_forest_doorway_step(field, state->origin, cached, end) ||
        !bfm_forest_ground(field, end[0], end[1], end[2], &ground) ||
        ground.normal[0] || ground.normal[2] || ground.height != end[1])
        return BFM_FOREST_MANUAL_BOUNDARY;

    for (unsigned axis = 0; axis < 3; axis += 2)
        next.position[axis] = ((uint32_t)(uint16_t)end[axis] << 16) |
                              (next.position[axis] & 0xffff);

    unsigned motion = speed < -(9 * Q16 + Q16 / 4) ? RUN : WALK;
    state->elapsed = motion == state->motion ? state->elapsed + 1 : 0;
    state->motion = motion;
    cached[0] = end[0];
    cached[2] = end[2];
    *actor = next;
    *yaw = next_yaw;
    return BFM_FOREST_MANUAL_MOVING;
}
