/* SPDX-License-Identifier: MIT */
#include "woods_blade_sparks.h"
#include "forest_bullet_art.h"

static int floor_q12(int value)
{
    return value / 4096 - (value % 4096 < 0);
}

void bfm_woods_blade_sparks_emit(BfmWoodsBladeSparks *sparks, const int32_t center[3],
                                const BfmForestTransform *blade, const int32_t eye[3],
                                unsigned random)
{
    if (!sparks || !center || !blade || !eye) return;
    for (unsigned i = 0; i < BFM_WOODS_BLADE_SPARKS; ++i) {
        BfmWoodsBladeSpark *spark = &sparks->slots[i];
        if (spark->remaining) continue;
        const int16_t origin[3] = {0};
        BfmForestTransform scatter;
        bfm_forest_actor_pitch_yaw((random & 127) * 32, ((random >> 8) & 127) * 32,
                                   origin, &scatter);
        bfm_forest_glow_facing(eye, center, 0, &spark->face);
        for (unsigned axis = 0; axis < 3; ++axis) {
            int product = 0;
            for (unsigned k = 0; k < 3; ++k)
                product += blade->rotation[axis][k] * scatter.rotation[k][2];
            int speed = floor_q12(2 * floor_q12(product));
            spark->velocity_q16[axis] = speed * 65536;
            spark->position_q16[axis] = (uint32_t)center[axis] << 16;
        }
        spark->remaining = BFM_WOODS_BLADE_SPARK_TICKS;
        return;
    }
}

void bfm_woods_blade_sparks_tick(BfmWoodsBladeSparks *sparks)
{
    if (!sparks) return;
    for (unsigned i = 0; i < BFM_WOODS_BLADE_SPARKS; ++i) {
        BfmWoodsBladeSpark *spark = &sparks->slots[i];
        if (!spark->remaining) continue;
        --spark->remaining;
        if (!spark->remaining) continue;
        /* Submit the current integral position before moving the particle. */
        for (unsigned axis = 0; axis < 3; ++axis) {
            unsigned coordinate = spark->position_q16[axis] >> 16;
            spark->face.translation[axis] = (int)(coordinate ^ 32768u) - 32768;
            spark->position_q16[axis] += (uint32_t)spark->velocity_q16[axis];
        }
        spark->velocity_q16[1] += 32768;
    }
}

size_t bfm_woods_blade_sparks_project(const BfmWoodsBladeSparks *sparks,
                        const BfmForestCamera *camera, BfmForestDustCard *cards, size_t capacity)
{
    if (!sparks || !camera || !cards) return 0;
    size_t count = 0;
    for (unsigned i = 0; i < BFM_WOODS_BLADE_SPARKS && count < capacity; ++i) {
        const BfmWoodsBladeSpark *spark = &sparks->slots[i];
        BfmForestCamera local;
        if (!spark->remaining || !bfm_forest_effect_camera(camera, &spark->face, &local)) continue;
        unsigned green = 192 - 16 * (BFM_WOODS_BLADE_SPARK_TICKS - spark->remaining);
        uint32_t rgb = (green / 4) | (green << 8) | ((green / 4) << 16);
        for (unsigned axis = 0; axis < 2 && count < capacity; ++axis) {
            int16_t corners[4][3] = {{0}};
            corners[0][axis] = 7;
            corners[3][axis] = -7;
            corners[1][1 - axis] = 2;
            corners[2][1 - axis] = -2;
            BfmForestDustCard card = {
                .material = {.page = 298, .count = 4, .translucent = 1},
                .slot = i * 2 + axis
            };
            card.vertices[1].rgb = card.vertices[2].rgb = rgb;
            if (bfm_forest_project_effect_quad(&local, corners, &card)) cards[count++] = card;
        }
    }
    return count;
}
