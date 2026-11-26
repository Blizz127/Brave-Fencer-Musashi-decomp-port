/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_CORE_DAMAGE_H
#define BFM_STEAM_KNIGHT_CORE_DAMAGE_H

#include "steam_knight_core_contact.h"

typedef struct {
    unsigned round, body_stage, weak_stage;
    uint32_t flags;
    int hp, max_hp;
    uint16_t pending_damage;
} BfmSteamKnightCoreState;

typedef enum {
    BFM_STEAM_CAP_STRUCK,
    BFM_STEAM_CORE_DAMAGED,
    BFM_STEAM_CORE_HALF_DEPLETED,
    BFM_STEAM_CORE_DEPLETED
} BfmSteamKnightCoreResponse;

typedef struct {
    BfmSteamKnightCoreContact contact;
    BfmSteamKnightCoreResponse response;
    int hp_after;
    uint16_t accepted;
    uint32_t clear_flags, set_flags;
} BfmSteamKnightCoreHit;

/* Reset struck on a new player swing. A successful encounter-owned call
 * receives one proposed update to its live body HP and flags. */
int bfm_steam_knight_core_strike(const BfmSteamKnightCoreState *body, unsigned *struck,
                                 const BfmSteamKnightCoreBoxes *boxes,
                                 const BfmForestTransform *attachment,
                                 const int16_t from[3], const int16_t ends[][3], size_t count,
                                 uint16_t strength, BfmSteamKnightCoreHit *hit);
#endif
