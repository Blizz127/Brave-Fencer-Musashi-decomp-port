/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_GUARD_CLIP_H
#define BFM_FOREST_GUARD_CLIP_H
#include "forest_field.h"

enum { BFM_GUARD_PARTS = 11, BFM_GUARD_POSE_BYTES = BFM_GUARD_PARTS * 8, BFM_GUARD_KEYS = 32 };
typedef struct {
    const uint8_t *pose[BFM_GUARD_KEYS];
    unsigned count;
} BfmForestGuardClip;

/* The owning field keeps the mounted one-tick poses alive. */
int bfm_forest_guard_clip_read(const BfmForestField *field, uint32_t list,
                               unsigned terminator, BfmForestGuardClip *clip);
#endif
