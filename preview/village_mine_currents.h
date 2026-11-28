/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_CURRENTS_H
#define BFM_VILLAGE_MINE_CURRENTS_H

#include "village_ground.h"

enum { BFM_MINE_CURRENT_COUNT = 5 };

typedef struct {
    int16_t x, z;
    unsigned profile;
} BfmMineCurrentSource;

typedef struct {
    BfmMineCurrentSource source[BFM_MINE_CURRENT_COUNT];
    const uint8_t *vectors;
    int16_t queued[3];
    int marked;
} BfmMineCurrents;

typedef struct {
    unsigned command, value;
} BfmMineCurrentSound;

/* Mount the flooded mine passage's five placed current sources in field order.
 * The scene remains mounted for the lifetime of this state. */
int bfm_mine_currents_load(BfmMineCurrents *currents, const uint8_t *scene, size_t bytes);

/* Take the previous actors' request once at the start of the player field.
 * The player's ground sweeps share this trial vector for that entire field.
 * An unmarked field returns zero and leaves vector alone. */
int bfm_mine_currents_take(BfmMineCurrents *currents, int16_t vector[3]);

/* Run one source after the player, or at its original placement visit during
 * construction. Bits in occupied name the five placed sources in order.
 * Only the sound-owning source writes sound; the third argument to the mixer is zero. */
void bfm_mine_currents_visit(BfmMineCurrents *currents, unsigned source, unsigned occupied,
    const BfmVillageGround *ground, const int16_t foot[3], BfmMineCurrentSound *sound);

/* Normal field with all five room placements present. */
void bfm_mine_currents_step(BfmMineCurrents *currents, const BfmVillageGround *ground,
    const int16_t foot[3], BfmMineCurrentSound *sound);

#endif
