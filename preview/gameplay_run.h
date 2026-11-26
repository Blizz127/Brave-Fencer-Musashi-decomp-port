/* SPDX-License-Identifier: MIT */
#ifndef BFM_GAMEPLAY_RUN_H
#define BFM_GAMEPLAY_RUN_H

#include "forest_status.h"
#include <stdint.h>

typedef struct {
    uint8_t figure_prices[128];
    uint8_t variables[256];
    uint8_t timestamps[96];
} BfmGameplayBanks;

typedef struct BfmGameplayRun {
    /* The original startup clears this word once. Rooms share the stream. */
    uint32_t random_state;
    uint8_t story_flags[64];
    /* The ability, its HUD label and current BP travel with this playthrough. */
    BfmForestStatus player;
    int has_player;
    BfmGameplayBanks banks;
} BfmGameplayRun;

/* Start the Chapter 1 banks from the mounted resident figure table. */
int bfm_gameplay_start_banks(BfmGameplayBanks *banks, const MusashiDiscMedia *disc);
/* The final Steam Knight cutscene advances the figure chapter before departure. */
int bfm_gameplay_advance_figures(BfmGameplayRun *run);

uint16_t bfm_gameplay_random(BfmGameplayRun *run);
int bfm_gameplay_story_flag(const BfmGameplayRun *run, unsigned flag);

/* Commit the summit award once to the current playthrough. */
int bfm_gameplay_acquire_lumina(BfmGameplayRun *run);

#endif
