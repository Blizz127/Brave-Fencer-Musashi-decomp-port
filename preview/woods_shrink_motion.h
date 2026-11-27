/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_SHRINK_MOTION_H
#define BFM_WOODS_SHRINK_MOTION_H

#include "musashi_disc_media.h"

typedef struct BfmWoodsShrinkMotion BfmWoodsShrinkMotion;
typedef enum BfmWoodsShrinkClip {
    BFM_SHRINK_WINDUP, BFM_SHRINK_RELEASE, BFM_SHRINK_EMPTY, BFM_SHRINK_RECOVER_FACE
} BfmWoodsShrinkClip;

/* Tables come from the mounted room player's body and face descriptors. */
BfmWoodsShrinkMotion *bfm_woods_shrink_motion_open(const MusashiDiscMedia *disc,
                                                  uint32_t primary_table, uint32_t face_table);
void bfm_woods_shrink_motion_free(BfmWoodsShrinkMotion *motion);
unsigned bfm_woods_shrink_motion_count(const BfmWoodsShrinkMotion *motion, BfmWoodsShrinkClip clip);
/* The complete 256-byte body packet is borrowed until motion is freed.
 * Advance one key per ordinary body update. RELEASE loops until the hold expires. */
const uint8_t *bfm_woods_shrink_motion_pose(const BfmWoodsShrinkMotion *motion,
                                           BfmWoodsShrinkClip clip, unsigned key);
/* An uploaded face uses word zero. EMPTY instead requests a resident facial move.
 * Apply the selected upload on entry and restore the normal face on exit. */
unsigned bfm_woods_shrink_motion_face_slot(const BfmWoodsShrinkMotion *motion, BfmWoodsShrinkClip clip);
uint32_t bfm_woods_shrink_motion_face_word(const BfmWoodsShrinkMotion *motion, BfmWoodsShrinkClip clip);
int bfm_woods_shrink_motion_face(const BfmWoodsShrinkMotion *motion, BfmWoodsShrinkClip clip,
                                uint16_t tiles[512][1024]);
#endif
