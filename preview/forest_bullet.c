/* SPDX-License-Identifier: MIT */
#include "forest_bullet.h"
#include <math.h>

enum { ONE = 4096, GUNSHOT = 8 };

static int floor_div(int64_t n, int divisor)
{
    return (int)(n / divisor - (n % divisor < 0));
}

/* The field restores unit length to every column after each rotation.
 * Its reciprocal square root uses a base-four mantissa. */
static void rotate(int matrix[3][3], unsigned axis, int angle)
{
    double radians = ((unsigned)angle & 4095) * (6.28318530717958647693 / ONE);
    int sine = (int)lround(sin(radians) * ONE), cosine = (int)lround(cos(radians) * ONE);
    unsigned a = (axis + 1) % 3, b = (axis + 2) % 3;
    for (unsigned row = 0; row < 3; ++row) {
        int u = matrix[row][a], v = matrix[row][b];
        matrix[row][a] = floor_div(u * cosine + v * sine, ONE);
        matrix[row][b] = floor_div(v * cosine - u * sine, ONE);
    }
    for (unsigned col = 0; col < 3; ++col) {
        unsigned mag = 0;
        for (unsigned row = 0; row < 3; ++row)
            mag += (unsigned)(matrix[row][col] * matrix[row][col]);
        unsigned divisor = 8;
        while (mag > 255) { mag >>= 2; divisor *= 2; }
        int gain = (int)sqrt((double)(1u << 30) / mag);
        for (unsigned row = 0; row < 3; ++row)
            matrix[row][col] = floor_div(matrix[row][col] * gain, divisor);
    }
}

int bfm_forest_bullet_launch(BfmForestBullets *pool, const BfmForestTransform *root,
                             int16_t pitch, int16_t yaw, int16_t roll)
{
    if (!pool || !root) return 0;
    unsigned slot = 0;
    while (slot < BFM_FOREST_BULLETS && pool->shots[slot].active) ++slot;
    if (slot == BFM_FOREST_BULLETS) return 0;

    int heading[3][3] = {{ONE,0,0},{0,ONE,0},{0,0,ONE}};
    rotate(heading, 0, pitch);
    rotate(heading, 2, roll);
    rotate(heading, 1, yaw);
    BfmForestBullet shot = {.remaining = 16, .active = 1};
    for (unsigned axis = 0; axis < 3; ++axis) {
        int offset = floor_div(-24 * root->rotation[axis][1] - 15 * root->rotation[axis][2], ONE);
        int64_t position = (int64_t)root->translation[axis] + offset;
        if (position < INT16_MIN || position > INT16_MAX) return 0;
        shot.position[axis] = (uint32_t)position << 16;
        shot.velocity[axis] = -1024 * heading[axis][2];
    }
    pool->shots[slot] = shot;
    return 1;
}

static int16_t world(uint32_t position)
{
    return (int16_t)((int32_t)((position >> 16) ^ UINT32_C(32768)) - 32768);
}

size_t bfm_forest_bullets_tick(BfmForestBullets *pool, unsigned ability,
                               BfmGunshotContact contact, void *context,
                               BfmBulletEvent events[BFM_FOREST_BULLETS])
{
    if (!pool) return 0;
    size_t count = 0;
    for (unsigned i = 0; i < BFM_FOREST_BULLETS; ++i) {
        BfmForestBullet *shot = &pool->shots[i];
        if (!shot->active) continue;
        if (ability != GUNSHOT || !shot->remaining) {
            shot->active = 0;
            continue;
        }
        BfmBulletEvent event = {.slot = i + 8, .kind = BFM_BULLET_FADE};
        for (unsigned a = 0; a < 3; ++a) event.position[a] = world(shot->position[a]);
        if (--shot->remaining) {
            int16_t from[3], to[3];
            for (unsigned a = 0; a < 3; ++a) {
                from[a] = event.position[a];
                shot->position[a] += (uint32_t)shot->velocity[a];
                to[a] = event.position[a] = world(shot->position[a]);
            }
            int hit = contact ? contact(context, from, to, event.position, event.normal) : 0;
            if (!hit) continue;
            shot->remaining = 0;
            event.kind = hit > 0 ? BFM_BULLET_ACTOR_HIT : BFM_BULLET_SURFACE_HIT;
        }
        if (events) events[count] = event;
        ++count;
    }
    return count;
}
