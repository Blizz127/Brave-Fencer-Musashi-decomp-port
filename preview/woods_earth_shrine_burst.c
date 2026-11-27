/* SPDX-License-Identifier: MIT */
#include "woods_earth_shrine_burst.h"
#include <math.h>

static int floor_q12(int value)
{
    return value / 4096 - (value % 4096 < 0);
}

int bfm_woods_earth_shrine_burst_start(BfmWoodsEarthShrineBurst *burst,
    const int projected_xy[2], unsigned (*random)(void *), void *context)
{
    if (!burst || !projected_xy || !random ||
        projected_xy[0] < INT16_MIN || projected_xy[0] > INT16_MAX ||
        projected_xy[1] < INT16_MIN || projected_xy[1] > INT16_MAX) return 0;
    burst->center[0] = (int16_t)projected_xy[0];
    burst->center[1] = (int16_t)projected_xy[1];
    for (unsigned i = 0; i < BFM_EARTH_BURST_RAYS; ++i) {
        BfmWoodsEarthShrineRay *ray = &burst->rays[i];
        ray->radius = 21 + random(context) % 6;
        int angle = (int)(256 * i + (random(context) & 63)) - 32;
        ray->length = 16 + (random(context) & 15);

        /* Motion uses the full sine wave. The triangle's rotation instead
         * uses the actor matrix's folded quarter wave, with signed input. */
        double radians = angle * (6.28318530717958647693 / 4096);
        ray->direction[0] = (int16_t)lround(4096 * cos(radians));
        ray->direction[1] = (int16_t)lround(4096 * sin(radians));
        BfmForestTransform rotation;
        const int16_t origin[3] = {0};
        bfm_forest_actor_root((unsigned)(angle < 0 ? -angle : angle), origin, &rotation);
        ray->rotation[0] = rotation.rotation[0][0];
        ray->rotation[1] = angle < 0 ? -rotation.rotation[0][2] : rotation.rotation[0][2];
    }
    burst->remaining = BFM_EARTH_BURST_UPDATES + 1;
    return 1;
}

void bfm_woods_earth_shrine_burst_step(BfmWoodsEarthShrineBurst *burst)
{
    if (burst && burst->remaining) --burst->remaining;
}

static void ray_card(const BfmWoodsEarthShrineBurst *burst, unsigned i,
                     unsigned elapsed, BfmForestDustCard *card)
{
    const BfmWoodsEarthShrineRay *ray = &burst->rays[i];
    int distance = (int)elapsed * 16;
    int scale = ((int)ray->radius + distance) * 64;
    int center[2], scaled[2], along[2], half[2];
    for (unsigned axis = 0; axis < 2; ++axis) {
        /* The starting offset is stored as whole pixels. Later displacement
         * accumulates at subpixel precision from that rounded position. */
        center[axis] = burst->center[axis] + floor_q12(ray->direction[axis] * (int)ray->radius)
                     + floor_q12(ray->direction[axis] * distance);
        scaled[axis] = floor_q12(ray->rotation[axis] * scale);
        along[axis] = floor_q12(scaled[axis] * (int)ray->length);
        half[axis] = along[axis] / 2 - (along[axis] % 2 < 0);
    }
    int side = (ray->length & 1) ? 1 : -1;
    *card = (BfmForestDustCard){
        .bucket = 16, .slot = i,
        .material = {.page = 0x20, .count = 3, .translucent = 1},
        .vertices = {
            {.x = center[0] + 3 * half[0], .y = center[1] + 3 * half[1], .rgb = 0x808080},
            {.x = center[0] - half[0], .y = center[1] - half[1], .rgb = 0x808080},
            {.x = center[0] + along[0] + floor_q12(-side * 2 * scaled[1]),
             .y = center[1] + along[1] + floor_q12(side * 2 * scaled[0]), .rgb = 0x808080}
        }
    };
}

int bfm_woods_earth_shrine_burst_append(const BfmWoodsEarthShrineBurst *burst,
    BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    if (!burst || !figure || !view) return 0;
    if (!burst->remaining || burst->remaining > BFM_EARTH_BURST_UPDATES) return 1;
    unsigned elapsed = BFM_EARTH_BURST_UPDATES - burst->remaining;
    BfmForestDustCard cards[BFM_EARTH_BURST_RAYS];
    for (unsigned i = 0; i < BFM_EARTH_BURST_RAYS; ++i)
        ray_card(burst, i, elapsed, &cards[i]);
    return bfm_bedroom_figure_add_fusion(figure, view, NULL, cards, BFM_EARTH_BURST_RAYS);
}
