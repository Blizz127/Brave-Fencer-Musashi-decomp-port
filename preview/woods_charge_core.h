/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_CHARGE_CORE_H
#define BFM_WOODS_CHARGE_CORE_H

#include "forest_dust.h"

enum { BFM_WOODS_CHARGE_CORE_CARDS = 10 };

typedef struct {
    const uint8_t *rim, *blade;
} BfmWoodsChargeCore;

/* The mounted Cleft Woods scene owns both contour streams. */
int bfm_woods_charge_core_load(BfmWoodsChargeCore *art, const uint8_t *scene, size_t bytes);

/* Submit once for a visual emitter that updated its tip this field. The tip is
 * its world midpoint before the camera nudge. The two signed local endpoints
 * and the actor model matrix come from that same player pose. The room counter
 * controls the pulse. An audio-only emitter has no mesh to submit. */
size_t bfm_woods_charge_core_project(const BfmWoodsChargeCore *art, const int32_t tip[3],
    const BfmForestTransform *model, const int8_t anchors[2][3], unsigned room_tick,
    unsigned gauge, const int32_t eye[3], const BfmForestCamera *camera,
    BfmForestDustCard *cards, size_t capacity);

#endif
