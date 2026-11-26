/* SPDX-License-Identifier: MIT */
#include "forest_shore_response.h"
#include <limits.h>

void bfm_forest_shore_response_init(BfmForestShoreResponse *body, int16_t health,
                                    uint16_t flags)
{
    *body = (BfmForestShoreResponse){.health = health, .flags = flags, .ordinary_flags = flags};
}

int bfm_forest_shore_response_queue(BfmForestShoreResponse *body, unsigned type,
                                    int power, int yaw)
{
    if (type < 1 || type > 3 || power < 0 || power > INT16_MAX ||
        (body->flags & 0x8100u) != 0x8000u || body->type == type || body->type == 10)
        return 0;
    body->type = (uint16_t)type;
    body->flags |= 1u;
    body->incoming_power = (int16_t)power;
    body->incoming_yaw = (uint16_t)yaw;
    return 1;
}

BfmForestShoreResponseStep bfm_forest_shore_response_tick(BfmForestShoreResponse *body)
{
    if (body->flags & 1u) {
        body->flags &= (uint16_t)~1u;
        /* Both shore figure variants take the full rolled normal attack strength. */
        body->health -= body->incoming_power;
        body->yaw = (body->incoming_yaw + 2048u) & 4095u;
        body->vertical_speed = 0;
        body->key = 0;
        body->phase = BFM_SHORE_RESPONSE_FLINCH;
        if (body->health <= 0) {
            body->flags = 0;
            body->phase = BFM_SHORE_RESPONSE_DEATH;
        }
    }
    if (body->phase == BFM_SHORE_RESPONSE_FLINCH && ++body->key == 30) {
        body->key = 0;
        body->phase = BFM_SHORE_RESPONSE_NORMAL;
        body->flags = body->ordinary_flags;
        body->type = 0;
        return BFM_SHORE_RESPONSE_RECOVER;
    }
    if (body->phase == BFM_SHORE_RESPONSE_DEATH && ++body->key == 20) {
        body->phase = BFM_SHORE_RESPONSE_REMOVE;
    }
    return body->phase;
}

int32_t bfm_forest_shore_response_height(BfmForestShoreResponse *body,
                                         int32_t y_q16, int16_t floor_y)
{
    int64_t speed = (int64_t)body->vertical_speed + 98304;
    int64_t next = y_q16 + speed;
    int32_t whole = (int32_t)((int64_t)y_q16 - (uint16_t)y_q16) / 65536;
    if (whole > floor_y) {
        speed = 0;
        next = (int32_t)floor_y * 65536 + (uint16_t)next;
    }
    if (speed > INT32_MAX || next < INT32_MIN || next > INT32_MAX) return y_q16;
    body->vertical_speed = (int32_t)speed;
    return (int32_t)next;
}
