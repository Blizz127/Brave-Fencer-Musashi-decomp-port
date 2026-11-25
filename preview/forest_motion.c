/* SPDX-License-Identifier: MIT */
#include "forest_motion.h"

#include <math.h>

enum { Q12 = 4096, LOCAL_LIMIT = 0x1f0000 };

static int32_t floor_q12(int64_t product)
{
    if (product < 0)
        return -(int32_t)((-product + Q12 - 1) / Q12);
    return (int32_t)(product / Q12);
}

int bfm_forest_level_velocity(ForestPlayerState *player, int16_t pitch,
                              int16_t yaw, int16_t roll)
{
    if (!player || pitch || roll)
        return 0;
    const int32_t local[3] = {player->field_24.word, player->local_y.word,
                              player->field_2c.word};
    for (unsigned axis = 0; axis < 3; ++axis)
        if (local[axis] < -LOCAL_LIMIT || local[axis] > LOCAL_LIMIT)
            return 0;

    const double radians_per_turn = 6.28318530717958647693;
    double angle = ((uint16_t)yaw % Q12) * (radians_per_turn / Q12);
    int32_t side = (int32_t)lround(sin(angle) * Q12);
    int32_t front = (int32_t)lround(cos(angle) * Q12);
    int32_t back = -side;
    /* The forest model expands a column when its quantized length falls
     * short of unity. The signed off-diagonal coefficients round separately. */
    if (side * side + front * front < Q12 * Q12) {
        const int scale = Q12 + 8;
        side = floor_q12((int64_t)side * scale);
        front = floor_q12((int64_t)front * scale);
        back = floor_q12((int64_t)back * scale);
    }
    int32_t x = floor_q12((int64_t)front * local[0] + (int64_t)side * local[2]);
    int32_t z = floor_q12((int64_t)back * local[0] + (int64_t)front * local[2]);
    player->velocity[0] = (uint32_t)((player->flags & 0x400000u) ? local[0] : x);
    player->velocity[1] = (uint32_t)local[1];
    player->velocity[2] = (uint32_t)((player->flags & 0x1000000u) ? local[2] : z);
    return 1;
}
