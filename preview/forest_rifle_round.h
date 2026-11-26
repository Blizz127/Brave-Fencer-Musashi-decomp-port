/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_RIFLE_ROUND_H
#define BFM_FOREST_RIFLE_ROUND_H
#include "forest_rifles.h"

typedef struct BfmForestRifleRoundArt BfmForestRifleRoundArt;
typedef struct {
    int64_t position_q16[3];
    int32_t velocity_q16[3];
    BfmForestTransform root;
    int16_t yaw;
    unsigned age;
    int active;
} BfmForestRifleRound;

/* The field and the mounted room must outlive this art. The sentries share it. */
BfmForestRifleRoundArt *bfm_forest_rifle_round_art_open(const BfmForestField *field);
void bfm_forest_rifle_round_art_free(BfmForestRifleRoundArt *art);
/* Sweep the next physical step before testing the last model against the player. */
int bfm_forest_rifle_round_blocked(const BfmForestRifleRound *round,
                                   const BfmForestField *field);

/* One level round from a bank sentry. Its first movement occurs as it is issued,
 * and the first model submission still comes from the muzzle. */
int bfm_forest_rifle_round_issue(BfmForestRifleRound *round,
                                 const BfmForestRifleRoundArt *art,
                                 const BfmForestRifleShot *shot);
/* Advance an existing round once per actor update. A confirmed collision
 * retires the round before another submission. The field owns hit responses. */
void bfm_forest_rifle_round_tick(BfmForestRifleRound *round,
                                 const BfmForestRifleRoundArt *art, int hit);
int bfm_forest_rifle_round_actor(const BfmForestRifleRound *round,
                                 const BfmForestRifleRoundArt *art, BfmForestActor *actor);
#endif
