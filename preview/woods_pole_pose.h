/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_POLE_POSE_H
#define BFM_WOODS_POLE_POSE_H

#include "bedroom_motion.h"
#include "woods_pole_climb.h"

typedef struct {
    const uint8_t *keys;
    unsigned count, face, sheathed;
} BfmWoodsPoleClip;

typedef struct {
    const uint8_t *main;
    BfmWoodsPoleClip climb, slide;
    unsigned key, sliding, armed, attached, sheathed;
} BfmWoodsPolePose;

/* Borrow expanded SC02/11 and the MAIN/10 resident payload for the life of
 * the room. A failed load leaves the previous controller intact. */
int bfm_woods_pole_pose_load(BfmWoodsPolePose *pose, const uint8_t *scene, size_t scene_bytes,
                             const uint8_t *main, size_t main_bytes);

/* Once per attached field, after the shared animation pass. Set caught on
 * each fresh contact. Up schedules one key change for the following field.
 * The ordinary jump/fall controller takes over as soon as contact ends. */
int bfm_woods_pole_pose_apply(BfmWoodsPolePose *pose, BfmBedroomMotion *ordinary,
                              BfmWoodsPoleMove move, int caught, int face_already_stepped,
                              BfmBedroomPose *frame);

#endif
