/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_SHRINK_SHOT_H
#define BFM_WOODS_SHRINK_SHOT_H
#include "forest_pose.h"
#include "forest_shot_flash.h"

enum { BFM_WOODS_SHRINK_SHOT_TICKS = 64, BFM_WOODS_SHRINK_EXPIRY_CARDS = 4 };

typedef struct {
    /* Frozen launch rotation, with its origin at the hand-level muzzle. */
    BfmForestTransform root;
    unsigned age, active;
    BfmForestShotFlash expiry;
} BfmWoodsShrinkShot;

/* Start with zeroed storage. Call begin instead of step on the launch field.
 * A recast replaces the ribbons and advances any existing stationary flash. */
void bfm_woods_shrink_shot_begin(BfmWoodsShrinkShot *shot,
                                 const BfmForestTransform *player_root);
void bfm_woods_shrink_shot_point(const BfmWoodsShrinkShot *shot,
                                 int16_t x, int16_t y, int16_t z, int16_t out[3]);

/* Step once per field, including after the ribbons expire. Pass the eligible
 * actor's feet, or NULL when none can receive Shrink. A hit leaves the
 * ribbons in flight. The final field emits flashes instead of a contact. */
int bfm_woods_shrink_shot_step(BfmWoodsShrinkShot *shot, unsigned equipped,
                               const int16_t eligible_target_root[3]);
size_t bfm_woods_shrink_shot_project_expiry(const BfmWoodsShrinkShot *shot,
    const BfmForestDust *atlas, const BfmForestCamera *camera, BfmForestDustCard *cards,
    size_t capacity);
#endif
