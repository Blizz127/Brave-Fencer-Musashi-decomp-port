/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_SHRINK_CAST_H
#define BFM_WOODS_SHRINK_CAST_H

#include "bedroom_motion.h"
#include "bedroom_travel.h"
#include "bedroom_figure.h"
#include "forest_status.h"

typedef struct BfmWoodsShrinkCast BfmWoodsShrinkCast;

typedef struct {
    int can_begin, circle_pressed, interrupted;
    const int16_t *target_root; /* Feet of an actor eligible to receive Shrink, or NULL. */
    const int32_t *eye;         /* Room camera position. */
    unsigned field_tick;
} BfmWoodsShrinkInput;

typedef struct {
    int movement_locked, paid, refused, target_contact;
    unsigned sound;             /* Emission edge; the wind-up has already spent its BP. */
    int player_lit;
    BfmBedroomLight lamps[3];   /* Replace daylight only when preparing the player's mesh. */
} BfmWoodsShrinkFrame;

BfmWoodsShrinkCast *bfm_woods_shrink_cast_open(const MusashiDiscMedia *disc,
                                               const uint8_t *scene, size_t bytes);
void bfm_woods_shrink_cast_free(BfmWoodsShrinkCast *cast);
/* Check before stepping locomotion. The new grounded Circle edge consumes
 * this update even if there is too little BP and only the refusal will play. */
int bfm_woods_shrink_cast_claims(const BfmWoodsShrinkCast *cast, const BfmForestStatus *status,
                                 const BfmWoodsShrinkInput *input);
/* Step once per simulation update. The outgoing projectile can hit after
 * movement resumes. A target_contact edge lets the room send its actor event. */
int bfm_woods_shrink_cast_step(BfmWoodsShrinkCast *cast, BfmForestStatus *status,
                               const BfmBedroomRoot *root, const BfmWoodsShrinkInput *input,
                               BfmWoodsShrinkFrame *frame);
/* Apply to the ordinary pose and facial tiles after the movement owner has
 * supplied them. Idle casts leave those outputs alone. */
void bfm_woods_shrink_cast_pose(const BfmWoodsShrinkCast *cast, BfmBedroomPose *body);
int bfm_woods_shrink_cast_face(const BfmWoodsShrinkCast *cast, uint16_t tiles[512][1024]);
int bfm_woods_shrink_cast_append(const BfmWoodsShrinkCast *cast, BfmBedroomFigure *figure,
                                 const BfmBedroomView *view);
#endif
