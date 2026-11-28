/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_SWITCHBACK_HIT_H
#define BFM_VILLAGE_SWITCHBACK_HIT_H

#include "field_input.h"
#include "village_mine_corridor_387.h"
#include "village_arrival_motion.h"

typedef struct BfmSwitchbackHit BfmSwitchbackHit;

BfmSwitchbackHit *bfm_switchback_hit_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, const int16_t feet[3]);
void bfm_switchback_hit_free(BfmSwitchbackHit *hit);
/* The hero receives the previous actor field's lunge before dispatching the pad. */
int bfm_switchback_hit_step(BfmSwitchbackHit *hit, BfmVillageMotion *motion, BfmGameplayRun *run,
    BfmSaveRecord *save, BfmFieldInput input, unsigned camera_yaw, const BfmMineLungeHit *incoming);
/* Publish the completed pose and the separate target seen by the sentry. */
const int16_t *bfm_switchback_hit_target(BfmSwitchbackHit *hit, BfmVillageMotion *motion);
int bfm_switchback_hit_body(const BfmSwitchbackHit *hit, const uint8_t *main,
    size_t bytes, BfmBedroomPose *pose, unsigned *visible);
int bfm_switchback_hit_draw(const BfmSwitchbackHit *hit, BfmBedroomFigure *figure,
    const BfmBedroomView *view);
unsigned bfm_switchback_hit_take_sound(BfmSwitchbackHit *hit);

#endif
