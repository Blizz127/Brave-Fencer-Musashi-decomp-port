/* SPDX-License-Identifier: MIT */
#ifndef BFM_BEDROOM_MOTION_H
#define BFM_BEDROOM_MOTION_H

#include <stddef.h>
#include <stdint.h>

typedef struct BfmBedroomMotion BfmBedroomMotion;

typedef enum {
    BFM_BEDROOM_NONE,
    BFM_BEDROOM_LIE,
    BFM_BEDROOM_RISE,
    BFM_BEDROOM_YAWN,
    BFM_BEDROOM_UPRIGHT,
    BFM_BEDROOM_WALK,
    BFM_BEDROOM_FALL,
    BFM_BEDROOM_LAND,
    BFM_BEDROOM_STAND,
    BFM_BEDROOM_REST,
    BFM_BEDROOM_SEARCH
} BfmBedroomAction;

typedef struct {
    const uint8_t *pose; /* 256 mounted bytes. The part transforms start at +16. */
    unsigned face_word;
    unsigned pitch; /* Model X rotation, in 1/4096 turns. */
    unsigned draw_current; /* The active scripted gesture submits its current pose. */
    unsigned finished; /* Player action completed in this update. */
} BfmBedroomPose;

/* The expanded room member and MAIN/10 type-0x101 payload (after its header)
 * are borrowed for the lifetime of the evaluator. */
BfmBedroomMotion *bfm_bedroom_motion_open(const uint8_t *scene, size_t scene_bytes,
                                          const uint8_t *main, size_t main_bytes);
int bfm_bedroom_motion_step(BfmBedroomMotion *motion, BfmBedroomAction action,
                            BfmBedroomPose *frame);
/* Apply a mounted facial texture and palette when the facial channel requests
 * an upload. Tagged in-VRAM facial moves are returned in frame.face_word. */
int bfm_bedroom_motion_face(const BfmBedroomMotion *motion, uint16_t tiles[512][1024]);
void bfm_bedroom_motion_free(BfmBedroomMotion *motion);

#endif
