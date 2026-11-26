/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_FINALE_H
#define BFM_STEAM_KNIGHT_FINALE_H

#include "steam_knight_event.h"
#include "forest_lift.h"

/* The fight supplies its resolved collision and the action that launched it.
 * Musashi himself can already be idle when the thrown chassis hits. */
typedef struct {
    unsigned round, phase, chassis_stage, chassis_hp;
    unsigned launch_action;
    int grounded_launch, struck_last_structure;
    int16_t hero[3];
} BfmSteamFinalImpact;

typedef enum {
    BFM_STEAM_FINAL_INACTIVE, BFM_STEAM_FINAL_THROW = 30,
    BFM_STEAM_FINAL_MACE, BFM_STEAM_FINAL_BLAST, BFM_STEAM_FINAL_WHITE,
    BFM_STEAM_FINAL_AFTERMATH
} BfmSteamFinalPhase;

typedef enum {
    BFM_STEAM_FINALE_PLAY, BFM_STEAM_FINALE_ENTERED_AFTERMATH,
    BFM_STEAM_FINALE_EXIT_START, BFM_STEAM_FINALE_RETIRED
} BfmSteamFinalSignal;

/* These edges come from the independently moving thrown chassis. */
typedef enum {
    BFM_STEAM_FINAL_NO_CONTACT, BFM_STEAM_FINAL_REPLAY, BFM_STEAM_FINAL_PIT_LANDING,
    BFM_STEAM_FINAL_LEDGE
} BfmSteamFinalBodyEvent;

typedef enum {
    BFM_STEAM_FINAL_HERO_IDLE, BFM_STEAM_FINAL_HERO_RAISE, BFM_STEAM_FINAL_HERO_HOLD,
    BFM_STEAM_FINAL_HERO_WALK, BFM_STEAM_FINAL_HERO_TOSS
} BfmSteamFinalHero;

typedef enum {
    BFM_STEAM_FINAL_MACE_AT_REST, BFM_STEAM_FINAL_MACE_HELD,
    BFM_STEAM_FINAL_MACE_AIRBORNE, BFM_STEAM_FINAL_MACE_HIT
} BfmSteamFinalMace;

typedef struct {
    BfmSteamFinalPhase phase;
    unsigned age, display_hp;
    int16_t hero[3];
    BfmSteamFinalHero hero_motion;
    BfmSteamFinalMace mace;
    unsigned motion_age;
    unsigned landed, ledge, page_open, page_closed_at, exit_started, retired, failed;
    const BfmForestLiftData *lift;
} BfmSteamKnightFinale;

int bfm_steam_finale_begin(BfmSteamKnightFinale *, const BfmSteamFinalImpact *, const BfmForestLiftData *);
/* Once per 30 Hz world update. This owns the normal voice step and the
 * event's message while active. One-shot signals go to the caller that owns
 * figures and the outgoing room. A negative result is an invalid stream. */
int bfm_steam_finale_tick(BfmSteamKnightFinale *, BfmSteamKnightEvent *, BfmSteamFinalBodyEvent);
int bfm_steam_finale_boss_hud(const BfmSteamKnightFinale *);
int bfm_steam_finale_crater_shot(const BfmSteamKnightFinale *);
/* Zero returns control of the body pose to the existing standing actor. */
uint32_t bfm_steam_finale_body(const BfmSteamKnightFinale *);
/* Composite after the field and its HUD, before the foreground dialogue. */
void bfm_steam_finale_flash(const BfmSteamKnightFinale *, uint32_t pixels[240][320]);

#endif
