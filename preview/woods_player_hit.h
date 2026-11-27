/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_PLAYER_HIT_H
#define BFM_WOODS_PLAYER_HIT_H

#include "bedroom_travel.h"
#include "forest_damage_digits.h"
#include "gameplay_run.h"

typedef struct BfmWoodsPlayerHit BfmWoodsPlayerHit;

typedef enum {
    BFM_WOODS_HIT_SAME_SUPPORT, BFM_WOODS_HIT_LOST_SUPPORT, BFM_WOODS_HIT_LANDED,
    BFM_WOODS_HIT_SURFACE_REACTION, BFM_WOODS_HIT_SOLID_ACTOR_REACTION
} BfmWoodsPlayerHitContact;

typedef struct {
    const BfmBedroomRoot *root;
    /* The preceding actor update's accepted overlap. Zero flags means no
     * contact. A status-only ring is present even when its power is zero. */
    unsigned hit_flags, power, attacker_yaw;
    uint32_t player_flags;
    /* Nonzero field action protection can mask hurt after guard selection. */
    uint32_t protected_action;
    /* Use the preceding completed field action. Guard-ready requires an
     * established stance. Entering the stance on this update is too late.
     * receives_damage is ordinary target reception with the scene mask clear. */
    int receives_damage, guard_ready, airborne, square_pressed, triangle_pressed;
    int carrying; /* Actual attachment, independent of transient saved carry bits. */
    unsigned player_action; /* The preceding field action. */
    /* Report only an actual new support loss, descending landing or a marked
     * surface during the reaction, from the field collision owner. */
    BfmWoodsPlayerHitContact contact;
} BfmWoodsPlayerHitInput;

typedef enum {
    BFM_WOODS_HIT_NO_ATTACK, BFM_WOODS_HIT_FUSION_ATTACK, BFM_WOODS_HIT_LUMINA_ATTACK, BFM_WOODS_HIT_CHARGED_ATTACK
} BfmWoodsPlayerGuardCancel;

typedef struct {
    uint32_t pose;
    const BfmForestDamageDigits *digits;
    unsigned sound;
    int hurt_started, guard_started, movement_locked, hide_body;
    /* Mini is read by the body/action gate before this field receives a new hit.
     * Its timer is stored inside run->player, including across saves. */
    int miniature, applied_mini, release_carry;
    int recovered, resume_fall, surface_reaction, solid_actor_reaction, zero_hp;
    BfmWoodsPlayerGuardCancel guard_cancel;
    unsigned cancel_countdown; /* Shared countdown at the accepted control edge. */
    /* Guard contact retains the preceding stance pose. Apply these Q16
     * world-space horizontal deltas through the ordinary field collision pass. */
    int32_t recoil_x, recoil_z;
} BfmWoodsPlayerHitFrame;

/* Reads the room's incoming-damage palette and the common player hurt keys
 * from the mounted US game. */
BfmWoodsPlayerHit *bfm_woods_player_hit_open(const MusashiDiscMedia *disc,
                                  const uint8_t *scene, size_t bytes);
void bfm_woods_player_hit_free(BfmWoodsPlayerHit *hit);
/* Step once before dispatching new player input. Hold ordinary movement on
 * locked frames. A zero-HP edge hands off to the field's real item/defeat
 * dispatcher, which owns any recovery, saved-data choice or transfer. */
int bfm_woods_player_hit_step(BfmWoodsPlayerHit *hit, BfmGameplayRun *run,
                        const BfmWoodsPlayerHitInput *input, BfmWoodsPlayerHitFrame *frame);
/* After a guard stride the field tests its corrected target against normal
 * terrain and actors. Report a newly selected fall, slope or actor action on
 * this same frame, before choosing the next movement or display state. */
void bfm_woods_player_hit_guard_contact(BfmWoodsPlayerHit *hit, BfmWoodsPlayerHitContact contact,
                                  BfmWoodsPlayerHitFrame *frame);
/* A moving support can finish an otherwise stationary hurt reaction early.
 * Report only the qualified contact from this update's field resolution. */
void bfm_woods_player_hit_hurt_contact(BfmWoodsPlayerHit *hit, BfmGameplayRun *run, uint32_t player_flags,
                                 BfmWoodsPlayerHitContact contact, BfmWoodsPlayerHitFrame *frame);
#endif
