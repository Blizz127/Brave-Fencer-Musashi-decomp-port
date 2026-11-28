/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_SHAFT_JUMP_BLADE_H
#define BFM_VILLAGE_MINE_SHAFT_JUMP_BLADE_H

#include "bedroom_motion.h"
#include "steam_knight_spin_contact.h"
#include "village_ground.h"
#include "woods_3026_box_hit.h"

typedef struct {
    BfmWeaponRank ranks[30];
    BfmWeaponPower power;
    unsigned event;
    uint16_t surface_sound[8], box_sound[2];
    unsigned effect, variant;
} BfmShaftBladeData;

typedef struct {
    BfmBedroomBodyCursor interrupted;
    unsigned hard_sound_played;
    unsigned box_contacts; /* Base and lid retain event 7 across swings in this room. */
} BfmShaftJumpBlade;

typedef struct {
    unsigned started, event, strength, rays;
    int16_t from[3], to[3][3];
} BfmShaftBladeFrame;

typedef struct {
    unsigned type, variant, sound;
    unsigned box_part; /* 1 for the base, 2 for the lid. */
    int16_t point[3], normal[3];
} BfmShaftBladeImpact;

/* Mount the airborne Fusion move and its stat calculation from SC03/109. */
int bfm_shaft_jump_blade_load(BfmShaftBladeData *data, const MusashiDiscMedia *disc,
                              const uint8_t *scene, size_t bytes);

/* The lake mounts its own move, power and surface sound rows. Its hit actors
 * receive the normal swing rays through the field and creature receivers. */
int bfm_lake_jump_blade_load(BfmShaftBladeData *data, const MusashiDiscMedia *disc,
                             const uint8_t *scene, size_t bytes);

/* Call after the ordinary body/face tick and before the shaft grip, landing
 * and fan. The ordinary airborne input slot owns the press. The supplied model
 * is the player's pre-move world transform. The action never moves the root.
 * A started swing requests the attached Fusion trail (type 4, variant 5), the
 * deduplicated slash cue 0x400 and the regular voice cue 0x5f3, in that order.
 * Resolve the returned rays against terrain then physical actors, in order. */
int bfm_shaft_jump_blade_step(BfmShaftJumpBlade *blade, const BfmShaftBladeData *data,
    BfmBedroomMotion *motion, BfmGameplayRun *run, const BfmForestTransform *model,
    int square_pressed, uint32_t actor_flags, BfmBedroomPose *body, BfmShaftBladeFrame *frame);

/* Resolve each segment against the room before its closed memory box. Pass
 * NULL for the box while a transfer or interaction has disabled its shapes.
 * Issue each hard cue before its spark. Submit all contact effects in ray
 * order, then issue any body-contact cues from those same rays. */
unsigned bfm_shaft_jump_blade_contacts(BfmShaftJumpBlade *blade, const BfmShaftBladeData *data,
    const BfmVillageGround *ground, const BfmWoods3026BoxHit *box,
    const BfmShaftBladeFrame *frame, BfmShaftBladeImpact impacts[3]);

/* If the ground catches the player during the strike, select the moving landing
 * program before cancelling. On a grip or other exit drop the interruption as
 * well. Each chest part retains its last event until a different weapon event
 * reaches that part or the room reconstructs the actor. */
void bfm_shaft_jump_blade_cancel(BfmShaftJumpBlade *blade);
int bfm_shaft_jump_blade_active(const BfmShaftJumpBlade *blade);
#endif
