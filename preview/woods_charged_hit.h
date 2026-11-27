/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_CHARGED_HIT_H
#define BFM_WOODS_CHARGED_HIT_H

#include "steam_knight_spin_contact.h"
#include "woods_caster_melee.h"

typedef struct BfmWoodsChargedHit {
    uint16_t percent, spread;
    uint8_t fusion_exp, lumina_exp;
    uint16_t surface_sound[8];
} BfmWoodsChargedHit;

typedef struct { uint8_t lumina, fusion; } BfmWoodsWeaponCredit;

typedef struct {
    BfmWoodsChargedHit hit;
    BfmWeaponRank ranks[30];
    BfmWoodsWeaponCredit credit;
} BfmWoodsChargedWeapon;

/* A wave can outlive its attack and uses the credit of the most recent
 * resolved weapon move. This view is owned by the room's melee controller. */
const BfmWoodsChargedWeapon *bfm_woods_caster_melee_weapon(const BfmWoodsCasterMelee *melee);

typedef struct {
    unsigned lumina, fusion; /* Increase in weapon stat on promotion. */
} BfmWoodsChargedLevels;

int bfm_woods_charged_hit_load(const uint8_t *scene, size_t bytes, BfmWoodsChargedHit *out);

/* Resolve once per attack update, including updates with the blade window
 * closed. The two tips are signed Lumina samples from consecutive body keys.
 * Only the contact fields in the caller's animation frame are written. */
int bfm_woods_charged_hit_step(const BfmWoodsChargedHit *hit,
    const BfmWeaponRank ranks[30], BfmGameplayRun *run, const BfmBedroomRoot *root,
    uint32_t actor_flags, int window, const int8_t tip[3], const int8_t next_tip[3],
    BfmWoodsCasterMeleeFrame *frame);

/* Call once after the moving wave has admitted an individual target. */
unsigned bfm_woods_charged_wave_damage(const BfmWoodsChargedHit *hit, BfmGameplayRun *run);

/* Each player weapon strength calculation replaces the two credit bytes.
 * A wave can hit after a new attack has replaced them. Keep a field-local
 * copy and apply it to each recipient that actually processes weapon EXP. */
BfmWoodsWeaponCredit bfm_woods_weapon_credit(const BfmGameplayRun *run,
                                           unsigned fusion, unsigned lumina);
int bfm_woods_weapon_credit_read(const uint8_t *scene, size_t bytes, unsigned event,
    const BfmGameplayRun *run, BfmWoodsWeaponCredit *credit);
int bfm_woods_weapon_credit_apply(const BfmWeaponRank ranks[30], BfmGameplayRun *run,
    BfmWoodsWeaponCredit credit, BfmWoodsChargedLevels *levels);

/* Direct event 11 blade contact, or a wave still carrying event 11's credit. */
int bfm_woods_charged_hit_connected(const BfmWoodsChargedHit *hit,
    const BfmWeaponRank ranks[30], BfmGameplayRun *run, BfmWoodsChargedLevels *levels);

#endif
