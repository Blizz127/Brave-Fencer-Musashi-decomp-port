/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_BANK_THORN_HURT_H
#define BFM_FOREST_BANK_THORN_HURT_H

#include "gameplay_run.h"
#include "village_ground.h"
#include "woods_caster_ring.h"

typedef struct {
    int16_t safe[3];
    unsigned phase, ticks;
    int32_t vertical_speed;
    uint16_t damage;
} BfmForestBankThornHurt;

typedef struct {
    const BfmWoodsCasterRingHit *incoming;
    const BfmVillageGround *ground;
    const MusashiDiscMedia *disc;
    /* The collision-corrected foot, in signed Q16, including fractions.
     * The receiver owns it on locked fields, including the finishing field. */
    int64_t *position;
    const BfmBedroomRoot *entrance;
    unsigned field, player_action;
    int pitch, yaw;
    uint32_t motion_flags, scene_state, protected_action, player_flags;
} BfmForestBankThornInput;

typedef struct {
    int started, movement_locked, hide_body, recovered, used_entrance, zero_hp;
    unsigned yaw, action, phase, sound, damage_number, bp_credit, blink_fields, ended_ability;
    /* Anchor for the hurt number (style 1), then any BP credit (style 4).
     * Both use the root before recoil and the room's mounted palette table. */
    int16_t effect_root[3];
    uint32_t rumble_program;
    /* Select the mounted direct body program on entry and continue its
     * ordinary clock until recovery. Recovery reenters standing with face 17. */
    uint32_t body_program;
} BfmForestBankThornFrame;

/* Start with zeroed state. Remember qualified ordinary footing, including
 * the first update after entry. A failed probe retains the previous foot. */
void bfm_forest_bank_thorn_remember(BfmForestBankThornHurt *hurt,
    const BfmVillageGround *ground, const int16_t foot[3]);

/* Step before ordinary control. Entry releases attachments and cues the
 * damage popup at the current hero root. Post-recovery blinking belongs to
 * common hit reception. A zero-HP edge enters its item/defeat check on the
 * next active field. */
int bfm_forest_bank_thorn_hurt_step(BfmForestBankThornHurt *hurt,
    BfmGameplayRun *run, const BfmForestBankThornInput *input,
    BfmForestBankThornFrame *frame);
#endif
