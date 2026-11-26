/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_CARRY_H
#define BFM_STEAM_KNIGHT_CARRY_H
#include "steam_knight_grip.h"
#include "steam_knight_hero_motion.h"
#include "steam_knight_machine.h"
#include "steam_knight_throw.h"

typedef struct {
    BfmSteamKnightThrow path;
    BfmSteamKnightGrip target;
    BfmForestLiftData lift;
    int held, thrown;
} BfmSteamKnightCarry;

int bfm_steam_carry_load(BfmSteamKnightCarry *, const MusashiDiscMedia *,
                         const uint8_t *scene, size_t bytes);
int bfm_steam_carry_knockdown(BfmSteamKnightCarry *, BfmSteamKnightMachine *, unsigned round);
int bfm_steam_carry_try_grab(BfmSteamKnightCarry *, BfmSteamKnightHeroMotion *, uint16_t pressed);
int bfm_steam_carry_check_brace(BfmSteamKnightCarry *, BfmSteamKnightHeroMotion *,
                                const int16_t previous[3]);
int bfm_steam_carry_tick(BfmSteamKnightCarry *, BfmSteamKnightHeroMotion *,
                         BfmSteamKnightMachine *, unsigned *events);
#endif
