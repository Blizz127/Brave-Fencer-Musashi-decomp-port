/* SPDX-License-Identifier: MIT */
#include "forest_manual.h"
#include "forest_action.h"
#include "forest_motion.h"
#include "forest_opening_data.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { WALK = 7, RUN = 8, Q16 = 65536, ACCELERATION = Q16 / 16 };

enum { ON_FLOOR, LEFT_LEDGE, LEFT_FIRST_CREEK_BANK };

/* Resolve a screen vector on the level ground. The camera can pitch, so
 * normalize its two projected axes separately before combining them. */
int bfm_forest_manual_heading(const BfmForestCamera *camera, int x, int y, int16_t *yaw)
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
static BfmForestTopStep step_on_floor(const BfmForestField *field,
    const BfmForestContact *ground, ForestPlayerState *next, int16_t cached[3], int can_fall,
    int on_bridge, int *left_creek)
{
    BfmForestTopStep support = BFM_FOREST_TOP_GROUNDED;
    uint32_t start_pos[3];
    memcpy(start_pos, next->position, sizeof start_pos);
    func_801473EC(next);
    int16_t end[3] = {(int16_t)(next->position[0] >> 16), cached[1],
                      (int16_t)(next->position[2] >> 16)};
    if (ground->bridge || on_bridge) {
        support = bfm_forest_step_on_bridge(field, cached, end);
        if (support != BFM_FOREST_TOP_GROUNDED)
            return can_fall ? support : BFM_FOREST_TOP_BLOCKED;
    } else if (ground->flags == 0x2000) {
        support = bfm_forest_raised_motion(field, ground, cached, end, start_pos, next->position);
        if (support == BFM_FOREST_TOP_BLOCKED ||
            (!can_fall && support != BFM_FOREST_TOP_GROUNDED))
            return BFM_FOREST_TOP_BLOCKED;
        if (support == BFM_FOREST_TOP_FREE)
            return support;
    } else if (left_creek && bfm_forest_walks_off_creek_bank(field, cached, end)) {
        *left_creek = 1;
        support = BFM_FOREST_TOP_LEFT;
    } else if (!bfm_forest_corridor_step(field, ground, cached, end)) {
        return BFM_FOREST_TOP_BLOCKED;
    }
    for (unsigned axis = 0; axis < 3; ++axis)
        next->position[axis] = ((uint32_t)(uint16_t)end[axis] << 16) |
                               (next->position[axis] & 0xffff);
    memcpy(cached, end, sizeof end);
    return support;
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
        step_on_floor(field, &ground, &next, cached, 0, 0, NULL) != BFM_FOREST_TOP_GROUNDED)
        return 0;
    *actor = next;
    return 1;
}

static BfmForestManualResult ground_control(BfmForestManual *state,
    const BfmForestField *field, ForestPlayerState *actor, const BfmForestCamera *camera,
    int screen_x, int screen_y, int16_t *yaw, int16_t cached[3], int32_t fixed_pace)
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
        state->after_jump = 0;
        state->motion = state->elapsed = 0;
        return result;
    }

    int16_t next_yaw;
    if (!bfm_forest_manual_heading(camera, screen_x, screen_y, &next_yaw))
        return BFM_FOREST_MANUAL_ERROR;
    if (!state->moving && !fixed_pace) {
        int32_t start = state->entry_velocity;
        if (!start) {
            BfmForestOpeningData data;
            if (!bfm_forest_opening_data(field, &data))
                return BFM_FOREST_MANUAL_ERROR;
            start = data.initial_velocity;
        }
        memset(actor->velocity, 0, sizeof actor->velocity);
        actor->field_24.word = actor->local_y.word = 0;
        actor->field_2c.word = start;
        actor->state = actor->phase = 1;
        memcpy(state->ground_normal, ground.normal, sizeof state->ground_normal);
        state->moving = 1;
        state->motion = WALK;
        state->elapsed = 0;
        return BFM_FOREST_MANUAL_STARTED;
    }

    ForestPlayerState next = *actor;
    int32_t speed = fixed_pace;
    if (!fixed_pace) {
        int32_t limit = state->after_jump ? 12 * Q16 : 11 * Q16;
        speed = next.field_2c.word;
        if (speed > 0 || speed < -limit)
            return BFM_FOREST_MANUAL_ERROR;
        speed -= ACCELERATION;
    }
    next.field_2c.word = speed;
    if (!bfm_forest_level_velocity(&next, 0, next_yaw, 0))
        return BFM_FOREST_MANUAL_ERROR;
    /* The world step uses this update's acceleration. Only the retained
     * local speed is subject to the run limit for the following update. */
    if (speed < -11 * Q16)
        speed = -11 * Q16;
    next.field_2c.word = speed;
    int left_creek = 0;
    BfmForestTopStep support = step_on_floor(field, &ground, &next, cached, state->last_descent > 0,
                                             state->on_bridge, fixed_pace ? NULL : &left_creek);
    if (support == BFM_FOREST_TOP_BLOCKED)
        return BFM_FOREST_MANUAL_BOUNDARY;

    if (support == BFM_FOREST_TOP_FREE) {
        state->left_support = LEFT_LEDGE;
        return BFM_FOREST_MANUAL_FALL;
    }
    unsigned motion = speed < -(9 * Q16 + Q16 / 4) ? RUN : WALK;
    state->elapsed = motion == state->motion ? state->elapsed + 1 : 0;
    state->moving = 1;
    state->motion = motion;
    state->after_jump = 0;
    state->on_bridge = ground.bridge || (state->on_bridge && abs(ground.height - cached[1]) > 4);
    state->left_support = support == BFM_FOREST_TOP_LEFT ? LEFT_LEDGE : ON_FLOOR;
    if (left_creek) state->left_support = LEFT_FIRST_CREEK_BANK;
    memcpy(state->ground_normal, ground.normal, sizeof state->ground_normal);
    *actor = next;
    *yaw = next_yaw;
    return BFM_FOREST_MANUAL_MOVING;
}

BfmForestManualResult bfm_forest_manual_tick(BfmForestManual *state,
    const BfmForestField *field, ForestPlayerState *actor, const BfmForestCamera *camera,
    int screen_x, int screen_y, int16_t *yaw, int16_t cached[3])
{
    return ground_control(state, field, actor, camera, screen_x, screen_y, yaw, cached, 0);
}

BfmForestManualResult bfm_forest_stance_step(BfmForestManual *state,
    const BfmForestField *field, ForestPlayerState *actor, const BfmForestCamera *camera,
    int screen_x, int screen_y, int16_t *yaw, int16_t cached[3])
{
    return ground_control(state, field, actor, camera, screen_x, screen_y, yaw, cached, -4 * Q16);
}

int bfm_forest_jump_begin(BfmForestJump *jump, const BfmForestField *field,
                          ForestPlayerState *actor, const int16_t cached[3])
{
    if (!jump || jump->active || !actor || !cached || actor->state > 1)
        return 0;
    BfmForestContact ground;
    int16_t probe[3] = {cached[0], cached[1], cached[2]};
    int hit;
    if (!bfm_forest_air_step(field, cached, probe, &ground, &hit) ||
        abs(ground.height - cached[1]) > 4 || !bfm_forest_jump_surface(&ground))
        return 0;
    /* A moving jump keeps its running momentum; a standing jump takes
     * its first directed step at nine units. */
    int32_t speed = actor->state ? actor->field_2c.word : -(9 * Q16 - ACCELERATION);
    if (speed > 0 || speed < -(11 * Q16 + 5 * Q16 / 8))
        return 0;
    *jump = (BfmForestJump){.active = 1, .ground = ground};
    actor->field_24.word = 0;
    actor->field_2c.word = speed;
    actor->local_y.word = -18 * Q16;
    actor->state = 2;
    return 1;
}

int bfm_forest_fall_begin(BfmForestJump *jump, BfmForestManual *manual,
                          ForestPlayerState *actor)
{
    if (!jump || jump->active || !manual || !manual->left_support || !actor ||
        actor->state != 1 || (manual->last_descent <= 0 && manual->left_support != LEFT_FIRST_CREEK_BANK))
        return 0;
    *jump = (BfmForestJump){.active = 1, .descending = 1, .steering = 1,
                            .walk_off = 1};
    manual->on_bridge = 0;
    manual->left_support = ON_FLOOR;
    actor->state = 3;
    actor->local_y.word = manual->last_descent > 0 ? manual->last_descent : 0;
    return 1;
}

BfmForestJumpResult bfm_forest_player_jump_tick(BfmForestJump *jump, const BfmForestField *field,
    ForestPlayerState *actor, const BfmForestCamera *camera, int x, int y, int cross_held,
    int16_t *yaw, int16_t cached[3], int has_dry_bank)
{
    if (!jump || !jump->active || !field || !actor || !camera || !yaw || !cached ||
        x < -1 || x > 1 || y < -1 || y > 1)
        return BFM_FOREST_JUMP_BOUNDARY;
    if (jump->landed) {
        ++jump->elapsed;
        ++jump->phase_elapsed;
        if (x || y || ++jump->recovery >= 10) {
            jump->active = 0;
            actor->state = x || y;
            actor->phase = 1;
            if (actor->state) {
                /* Ground locomotion starts from the whole, signed part of
                 * the airborne forward velocity. Keep position fractions. */
                actor->field_2c.halves.low = 0;
            } else {
                actor->field_2c.word = 0;
            }
            return BFM_FOREST_JUMP_FINISHED;
        }
        return BFM_FOREST_JUMP_LANDED;
    }

    BfmForestJump next_jump = *jump;
    ForestPlayerState next = *actor;
    int16_t next_yaw = *yaw;
    if (x || y) {
        if (!bfm_forest_manual_heading(camera, x, y, &next_yaw))
            return BFM_FOREST_JUMP_BOUNDARY;
        next_jump.steering = 1;
        next_jump.neutral_ticks = 0;
    } else if (++next_jump.neutral_ticks > 3) {
        next_jump.steering = 0;
    }
    const int32_t max_speed = 11 * Q16 + 5 * Q16 / 8;
    int32_t speed = next.field_2c.word;
    if (x || y) {
        speed -= ACCELERATION;
        if (speed < -max_speed) speed = -max_speed;
    }
    next.field_2c.word = next_jump.steering ? speed : 0;
    /* On the first unsupported update the actor continues across the rim.
     * Its preceding touchdown supplies the vertical speed on the next one. */
    int leaving_rim = next_jump.walk_off && !next_jump.elapsed;
    int32_t retained_descent = next.local_y.word;
    if (leaving_rim)
        next.local_y.word = 0;
    else
        next.local_y.word += !cross_held && next.local_y.word < 0 ? 21 * Q16 / 8 : 13 * Q16 / 8;
    if (!bfm_forest_level_velocity(&next, 0, next_yaw, 0))
        return BFM_FOREST_JUMP_BOUNDARY;
    /* The stride leaving the rim still uses grounded acceleration. Carry
     * its capped running speed into the first downward step. */
    if (leaving_rim && speed < -11 * Q16)
        speed = -11 * Q16;
    next.field_2c.word = speed;
    func_801473EC(&next);
    int16_t end[3];
    for (unsigned a = 0; a < 3; ++a)
        end[a] = (int16_t)(next.position[a] >> 16);
    int landed, water = 0;
    int *water_contact = has_dry_bank ? &water : NULL;
    if (!bfm_forest_player_air_step(field, cached, end, &next_jump.ground, &landed, water_contact)) {
        /* Keep falling in the last supported column when a lateral route
         * leads into terrain that this movement path cannot resolve. */
        end[0] = cached[0];
        end[2] = cached[2];
        if (!bfm_forest_player_air_step(field, cached, end, &next_jump.ground, &landed, water_contact))
            return BFM_FOREST_JUMP_BOUNDARY;
        next.position[0] = actor->position[0];
        next.position[2] = actor->position[2];
        next.velocity[0] = next.velocity[2] = 0;
    }
    if (leaving_rim)
        next.local_y.word = retained_descent;
    else
        ++next_jump.phase_elapsed;
    if (!next_jump.descending && next.local_y.word >= 0) {
        next_jump.descending = 1;
        next_jump.phase_elapsed = 0;
    }
    for (unsigned a = 0; a < 3; ++a)
        next.position[a] = ((uint32_t)(uint16_t)end[a] << 16) | (next.position[a] & 0xffff);
    if (landed) {
        next_jump.touchdown_velocity = next.local_y.word;
        next.local_y.word = 0;
        memset(next.velocity, 0, sizeof next.velocity);
        next_jump.landed = 1;
        next_jump.phase_elapsed = 0;
    }
    ++next_jump.elapsed;
    *jump = next_jump;
    *actor = next;
    *yaw = next_yaw;
    memcpy(cached, end, sizeof end);
    if (water) return BFM_FOREST_JUMP_WATER;
    return landed ? BFM_FOREST_JUMP_LANDED : BFM_FOREST_JUMP_AIRBORNE;
}

BfmForestJumpResult bfm_forest_jump_tick(BfmForestJump *jump, const BfmForestField *field,
    ForestPlayerState *actor, const BfmForestCamera *camera, int x, int y, int cross_held,
    int16_t *yaw, int16_t cached[3])
{
    return bfm_forest_player_jump_tick(jump, field, actor, camera, x, y, cross_held, yaw, cached, 0);
}

void bfm_forest_jump_resume(BfmForestManual *manual, const BfmForestJump *jump,
                            const ForestPlayerState *actor)
{
    if (!manual || !jump || !actor || jump->active || !jump->landed)
        return;
    *manual = (BfmForestManual){.last_descent = jump->touchdown_velocity, .on_bridge = jump->ground.bridge};
    memcpy(manual->ground_normal, jump->ground.normal, sizeof manual->ground_normal);
    /* The first subsequent step selects the locomotion clip at frame zero. */
    if (actor->state == 1)
        manual->moving = manual->after_jump = 1;
}

int bfm_forest_carry_step(const BfmForestField *field, ForestPlayerState *actor, int16_t yaw,
                          int32_t speed, int16_t position[3])
{
    BfmForestContact ground;
    if (!bfm_forest_ground(field, position[0], position[1], position[2], &ground)) return 0;
    ForestPlayerState next = *actor;
    next.field_24.word = next.local_y.word = 0;
    next.field_2c.word = speed;
    if (!bfm_forest_level_velocity(&next, 0, yaw, 0) ||
        step_on_floor(field, &ground, &next, position, 0, ground.bridge, NULL) != BFM_FOREST_TOP_GROUNDED) return 0;
    *actor = next;
    return 1;
}
