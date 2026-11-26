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

/* Resolve the free Q16 position on the registered room floor. Only commit
 * the integrated actor and its integer position after a complete step. */
static int step_on_floor(const BfmForestField *field, const BfmForestContact *ground,
                         ForestPlayerState *next, int16_t cached[3])
{
    func_801473EC(next);
    int16_t end[3] = {(int16_t)(next->position[0] >> 16), cached[1],
                      (int16_t)(next->position[2] >> 16)};
    if (!bfm_forest_corridor_step(field, ground, cached, end))
        return 0;
    for (unsigned axis = 0; axis < 3; ++axis)
        next->position[axis] = ((uint32_t)(uint16_t)end[axis] << 16) |
                               (next->position[axis] & 0xffff);
    memcpy(cached, end, sizeof end);
    return 1;
}

int bfm_forest_manual_coast(const BfmForestField *field, ForestPlayerState *actor,
                            int16_t yaw, int16_t cached[3])
{
    BfmForestContact ground;
    if (!field || !actor || !cached ||
        !bfm_forest_ground(field, cached[0], cached[1], cached[2], &ground))
        return 0;
    ForestPlayerState next = *actor;
    if (!bfm_forest_level_velocity(&next, 0, yaw, 0) ||
        !step_on_floor(field, &ground, &next, cached))
        return 0;
    *actor = next;
    return 1;
}

BfmForestManualResult bfm_forest_manual_tick(BfmForestManual *state,
    const BfmForestField *field, ForestPlayerState *actor, const BfmForestCamera *camera,
    int screen_x, int screen_y, int16_t *yaw, int16_t cached[3])
{
    if (!state || !field || !actor || !camera || !yaw || !cached ||
        screen_x < -1 || screen_x > 1 || screen_y < -1 || screen_y > 1)
        return BFM_FOREST_MANUAL_ERROR;

    BfmForestContact ground;
    if (!bfm_forest_ground(field, cached[0], cached[1], cached[2], &ground))
        return BFM_FOREST_MANUAL_BOUNDARY;

    if (!screen_x && !screen_y) {
        BfmForestManualResult result = BFM_FOREST_MANUAL_IDLE;
        if (state->moving)
            result = actor->field_2c.word == -11 * Q16 ?
                     BFM_FOREST_MANUAL_RUN_STOPPED : BFM_FOREST_MANUAL_STOPPED;
        memcpy(state->ground_normal, ground.normal, sizeof state->ground_normal);
        memset(actor->velocity, 0, sizeof actor->velocity);
        actor->field_24.word = 0;
        actor->local_y.word = 0;
        actor->field_2c.word = 0;
        actor->state = 0;
        actor->phase = 1;
        state->moving = 0;
        state->motion = state->elapsed = 0;
        return result;
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
        memcpy(state->ground_normal, ground.normal, sizeof state->ground_normal);
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
    next.field_2c.word = speed;
    if (!bfm_forest_level_velocity(&next, 0, next_yaw, 0))
        return BFM_FOREST_MANUAL_ERROR;
    /* The world step uses this update's acceleration. Only the retained
     * local speed is subject to the run limit for the following update. */
    if (speed < -11 * Q16)
        speed = -11 * Q16;
    next.field_2c.word = speed;
    if (!step_on_floor(field, &ground, &next, cached))
        return BFM_FOREST_MANUAL_BOUNDARY;

    unsigned motion = speed < -(9 * Q16 + Q16 / 4) ? RUN : WALK;
    state->elapsed = motion == state->motion ? state->elapsed + 1 : 0;
    state->motion = motion;
    memcpy(state->ground_normal, ground.normal, sizeof state->ground_normal);
    *actor = next;
    *yaw = next_yaw;
    return BFM_FOREST_MANUAL_MOVING;
}
