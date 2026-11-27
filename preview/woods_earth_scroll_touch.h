/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_EARTH_SCROLL_TOUCH_H
#define BFM_WOODS_EARTH_SCROLL_TOUCH_H

#include "forest_guard_contact.h"
#include <stddef.h>
#include <stdint.h>

typedef enum {
    BFM_EARTH_TOUCH_ORDINARY,
    BFM_EARTH_TOUCH_ATTACHED
} BfmEarthTouchHeroBody;

typedef struct {
    BfmForestGuardBox hero_body[2];
    int16_t edge_x[2], edge_y, edge_z[2];
    int mounted;
} BfmWoodsEarthScrollTouch;

/* Read the loose emblem's two pickup edges and Musashi's ordinary body
 * collider from the mounted Twinpeak scene. Failure preserves out. */
int bfm_woods_earth_scroll_touch_mount(const uint8_t *scene, size_t bytes,
                                      BfmWoodsEarthScrollTouch *out);

/* Call during the loose emblem's contact phase with both last completed live
 * model transforms. The emblem's basis moves its edges into the world. The
 * ordinary player box follows the model origin in world axes, including the
 * vertical motion of the jump. Select ATTACHED only while the player uses
 * the alternate rotated collider during action 12. */
int bfm_woods_earth_scroll_touch_test(const BfmWoodsEarthScrollTouch *touch,
                                     const BfmForestTransform *emblem_model,
                                     const BfmForestTransform *hero_model,
                                     BfmEarthTouchHeroBody body_kind);
#endif
