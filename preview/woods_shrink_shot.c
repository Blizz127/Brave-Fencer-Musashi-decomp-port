/* SPDX-License-Identifier: MIT */
#include "woods_shrink_shot.h"
#include <limits.h>

enum { SHRINK = 13, MATRIX_ONE = 4096, CONTACT_RADIUS = 64 };

void bfm_woods_shrink_shot_begin(BfmWoodsShrinkShot *shot,
                                 const BfmForestTransform *player_root)
{
    if (!shot || !player_root) return;
    bfm_forest_shot_flash_advance(&shot->expiry);
    shot->root = *player_root;
    int16_t muzzle[3];
    bfm_woods_shrink_shot_point(shot, 0, -24, -15, muzzle);
    for (unsigned axis = 0; axis < 3; ++axis)
        shot->root.translation[axis] = muzzle[axis];
    shot->age = 0;
    shot->active = 1;
}

void bfm_woods_shrink_shot_point(const BfmWoodsShrinkShot *shot,
                                 int16_t x, int16_t y, int16_t z, int16_t out[3])
{
    if (!shot || !out) return;
    for (unsigned axis = 0; axis < 3; ++axis) {
        const int16_t *row = shot->root.rotation[axis];
        int64_t delta = (int64_t)row[0] * x + (int64_t)row[1] * y + (int64_t)row[2] * z;
        int64_t position = shot->root.translation[axis] + delta / MATRIX_ONE;
        if (delta % MATRIX_ONE < 0) --position;
        if (position < INT16_MIN) position = INT16_MIN;
        if (position > INT16_MAX) position = INT16_MAX;
        out[axis] = (int16_t)position;
    }
}

int bfm_woods_shrink_shot_step(BfmWoodsShrinkShot *shot, unsigned equipped,
                               const int16_t eligible_target_root[3])
{
    if (!shot) return 0;
    bfm_forest_shot_flash_advance(&shot->expiry);
    if (!shot->active) return 0;
    if (equipped != SHRINK || shot->age >= BFM_WOODS_SHRINK_SHOT_TICKS) {
        shot->active = 0;
        return 0;
    }
    ++shot->age;
    int16_t depth = (int16_t)(-15 - 6 * (int)shot->age);
    if (shot->age == BFM_WOODS_SHRINK_SHOT_TICKS) {
        int16_t tip[3];
        bfm_woods_shrink_shot_point(shot, 0, -24, depth, tip);
        bfm_forest_shot_flash_start(&shot->expiry, BFM_SHOT_FLASH_EXPIRY, tip, NULL);
        return 0;
    }
    if (!eligible_target_root) return 0;

    /* The probe follows the centerline from the muzzle. The visible ribbons
     * travel above it. Both continue along the original heading. */
    int16_t probe[3];
    bfm_woods_shrink_shot_point(shot, 0, 0, depth, probe);
    int64_t distance2 = 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        int delta = eligible_target_root[axis] - probe[axis];
        distance2 += (int64_t)delta * delta;
    }
    return distance2 < CONTACT_RADIUS * CONTACT_RADIUS;
}

size_t bfm_woods_shrink_shot_project_expiry(const BfmWoodsShrinkShot *shot,
    const BfmForestDust *atlas, const BfmForestCamera *camera, BfmForestDustCard *cards,
    size_t capacity)
{
    if (!shot || !cards || !capacity ||
        !bfm_forest_shot_flash_project(&shot->expiry, atlas, camera, &cards[0]))
        return 0;
    /* Each strand releases the same stationary additive sprite. They have
     * identical lifetimes; overlapping them is part of the flash's intensity. */
    size_t count = capacity < BFM_WOODS_SHRINK_EXPIRY_CARDS ?
                   capacity : BFM_WOODS_SHRINK_EXPIRY_CARDS;
    for (size_t i = 1; i < count; ++i) cards[i] = cards[0];
    return count;
}
