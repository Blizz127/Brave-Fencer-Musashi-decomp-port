/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_CARRY_H
#define BFM_WOODS_CARRY_H

#include "forest_lift.h"
#include "gameplay_run.h"

typedef struct {
    BfmForestLiftData data;
    BfmForestLift lift;
    int16_t grip[3];
    unsigned brace_age;
    int braced, triangle_throw, released;
} BfmWoodsCarry;

typedef enum {
    BFM_CARRY_NONE, BFM_CARRY_LIFT, BFM_CARRY_THROW, BFM_CARRY_DROP
} BfmCarryEvent;

typedef struct BfmWoodsCarryFrame {
    uint32_t pose;
    unsigned face_slot;  /* Zero continues the current face program. */
    int turn, started_throw, attached, finished;
    int32_t speed_q16;   /* Positive forward speed for the village mover. */
    BfmCarryEvent event;
    unsigned interaction_power;
    int16_t socket[3];
} BfmWoodsCarryFrame;

int bfm_woods_carry_load(BfmWoodsCarry *carry, const MusashiDiscMedia *disc,
                         const uint8_t *scene, size_t size);
/* After ordinary walking and the actor sweep, apply the pushing pose if the
 * body caught the foot segment at Y-16. A miss clears the previous brace. */
int bfm_woods_carry_brace(BfmWoodsCarry *carry, int contacted, BfmWoodsCarryFrame *frame);
/* Offer the end of the original pickup segment only on an eligible fresh
 * button edge in the preceding brace. Test the segment against the live dog. */
int bfm_woods_carry_reach(const BfmWoodsCarry *carry, const BfmForestTransform *hero,
                          int square, int triangle, int lumina, int16_t end[3]);
int bfm_woods_carry_begin(BfmWoodsCarry *carry, BfmGameplayRun *run,
                          BfmWoodsCarryFrame *frame);
/* Once per player update after pickup. The socket uses the previous published
 * transform. The room applies the movement and the dog's new facing once. */
int bfm_woods_carry_step(BfmWoodsCarry *carry, BfmGameplayRun *run,
                         const BfmForestTransform *hero, int directed, int square,
                         int triangle, int lumina, BfmWoodsCarryFrame *frame);
int bfm_woods_carry_active(const BfmWoodsCarry *carry);
/* A lost footing or accepted hit releases a held actor before ordinary fall
 * or hurt. An already released throw must never generate a second event. */
BfmCarryEvent bfm_woods_carry_interrupt(BfmWoodsCarry *carry);
#endif
