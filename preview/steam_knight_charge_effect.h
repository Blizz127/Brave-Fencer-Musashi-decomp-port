/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_CHARGE_EFFECT_H
#define BFM_STEAM_KNIGHT_CHARGE_EFFECT_H

#include "forest_dust.h"
#include "gameplay_run.h"

enum { BFM_STEAM_CHARGE_SLOTS = 30, BFM_STEAM_CHARGE_EMITTER = 20,
       BFM_STEAM_CHARGE_STREAK = 23 };

typedef struct {
    uint8_t kind, started, parent, age, full, sound_only;
    uint32_t timer;
    int16_t previous[3];
    int32_t tip[3];
} BfmSteamChargeSlot;

typedef struct {
    int16_t from[3], to[3];
    unsigned slot;
} BfmSteamChargeLine;

typedef struct {
    BfmSteamChargeSlot slots[BFM_STEAM_CHARGE_SLOTS];
    BfmSteamChargeLine lines[BFM_STEAM_CHARGE_SLOTS];
    unsigned line_count;
    /* Slot bits for visible sword centers updated during the last field. */
    uint32_t centers;
    /* Per-update sound requests: start or stop the charge cue, pulse the full cue. */
    int play_charge, stop_charge, play_full;
} BfmSteamChargeEffects;

/* Start with a zeroed pool. External effect owners can claim their actual kind
 * from the same ascending free list and release that exact slot on retirement. */
int bfm_steam_charge_reserve(BfmSteamChargeEffects *effects, uint8_t kind);
void bfm_steam_charge_release(BfmSteamChargeEffects *effects, unsigned slot, uint8_t kind);

/* A newly entered charge uses sound alone when no scroll is equipped. */
int bfm_steam_charge_enter(BfmSteamChargeEffects *effects, uint8_t equipped_scroll);
/* Advance once per gameplay update, including release/cancellation at gauge zero.
 * The two signed triples start at byte eight of the active pose packet header. */
int bfm_steam_charge_step(BfmSteamChargeEffects *effects, BfmGameplayRun *run,
                          unsigned gauge, const BfmForestTransform *model,
                          const int8_t anchors[2][3]);
/* A room with other effect kinds advances each of those owners at its own
 * slot visit. Its callback can release or allocate slots in this same pool. */
typedef void (*BfmSteamChargeOtherEffect)(void *context, unsigned slot, uint8_t kind);
int bfm_steam_charge_step_shared(BfmSteamChargeEffects *effects, BfmGameplayRun *run,
                                 unsigned gauge, const BfmForestTransform *model,
                                 const int8_t anchors[2][3],
                                 BfmSteamChargeOtherEffect visit, void *context);
unsigned bfm_steam_charge_project(const BfmSteamChargeEffects *effects,
                                  const BfmForestCamera *camera,
                                  BfmForestDustCard *cards, unsigned capacity);
#endif
