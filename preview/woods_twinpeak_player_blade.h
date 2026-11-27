/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_TWINPEAK_PLAYER_BLADE_H
#define BFM_WOODS_TWINPEAK_PLAYER_BLADE_H

#include "village_arrival_motion.h"
#include "woods_caster_melee.h"

int bfm_twinpeak_player_field_ready(const BfmVillageMotion *motion, const BfmGameplayRun *run);

/* The room mounts its ordinary blade with bfm_woods_twinpeak_melee_open and
 * retains a zero-initialized frame between updates. Pass field_free = 0 when
 * dialogue, carry, injury, resting/charged action or a scene owns the input.
 * Give ordinary interactions first refusal of Square. A nonzero action claims
 * physical movement for this field. Hold the grounded feet/yaw instead of
 * advancing ordinary walking; dispatch sound and active rays exactly once. */
int bfm_twinpeak_player_blade_step(BfmWoodsCasterMelee *blade, const BfmVillageMotion *motion,
    BfmSaveRecord *save, BfmGameplayRun *run, BfmFieldInput input, int field_free,
    BfmWoodsCasterMeleeFrame *swing);

/* Overlay the sword bank's delayed body and weapon display on a copy of the
 * current field pose. The motion retains ownership of feet, shadow and face. */
int bfm_twinpeak_player_blade_pose(const BfmWoodsCasterMeleeFrame *swing,
    const uint8_t *main, size_t bytes, BfmBedroomPose *body);

#endif
