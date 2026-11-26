/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_RIFLES_H
#define BFM_FOREST_RIFLES_H

#include "forest_field.h"

enum { BFM_FOREST_BANK_RIFLES = 3 };
typedef struct BfmForestRifles BfmForestRifles;

typedef struct {
    int16_t muzzle[3], yaw;
} BfmForestRifleShot;

BfmForestRifles *bfm_forest_rifles_open(const BfmForestField *field);
void bfm_forest_rifles_free(BfmForestRifles *rifles);

/* Scan the ordinary bank placements after updating the live room actors. */
int bfm_forest_rifles_place(BfmForestRifles *rifles, int player_z);
/* Advance an undisturbed bank sentry at its place in the actor update order.
 * A positive return of two issues a bullet at shot. Zero indicates bad data. */
int bfm_forest_rifle_tick(BfmForestRifles *rifles, unsigned index, int player_z,
                          BfmGameplayRun *run, BfmForestRifleShot *shot);
const BfmForestActor *bfm_forest_rifle_actor(const BfmForestRifles *rifles, unsigned index);
/* The two-update flash is self-lit. Its transform contains the rolled axial
 * rotation. The returned luma belongs to the two companion light fans. */
const BfmForestActor *bfm_forest_rifle_flash(const BfmForestRifles *rifles,
                                             unsigned index, unsigned *luma);
#endif
