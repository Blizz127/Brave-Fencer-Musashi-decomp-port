/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_SHORE_RESPONSE_H
#define BFM_FOREST_SHORE_RESPONSE_H

#include <stdint.h>

typedef enum {
    BFM_SHORE_RESPONSE_NORMAL,
    BFM_SHORE_RESPONSE_FLINCH,
    BFM_SHORE_RESPONSE_RECOVER,
    BFM_SHORE_RESPONSE_DEATH,
    BFM_SHORE_RESPONSE_REMOVE
} BfmForestShoreResponseStep;

typedef struct {
    BfmForestShoreResponseStep phase;
    int16_t health;
    uint16_t flags, type, yaw;
    unsigned key;
    int32_t vertical_speed;
    uint16_t ordinary_flags, incoming_yaw;
    int16_t incoming_power;
} BfmForestShoreResponse;

/* Initialize a mounted figure with its difficulty-adjusted starting health and flags. */
void bfm_forest_shore_response_init(BfmForestShoreResponse *body, int16_t health,
                                    uint16_t flags);

/* Queue an ordinary Fusion hit after the blade has intersected a body box.
 * A later eligible ray can replace a pending contact before the actor runs. */
int bfm_forest_shore_response_queue(BfmForestShoreResponse *body, unsigned type,
                                    int power, int yaw);

/* Once per actor update. FLINCH and DEATH select the mounted hit/death clip at
 * key. RECOVER asks the ordinary owner to enter its ready state at key zero.
 * REMOVE releases the actor; its linked base observes that release separately. */
BfmForestShoreResponseStep bfm_forest_shore_response_tick(BfmForestShoreResponse *body);

/* Vertical component of a flinching target's grounded step on a flat bank.
 * Pass the current signed Q16 world height and the mounted floor's whole
 * height. X/Z are stationary during this response. */
int32_t bfm_forest_shore_response_height(BfmForestShoreResponse *body,
                                         int32_t y_q16, int16_t floor_y);

#endif
