/* SPDX-License-Identifier: MIT */
#ifndef BFM_GAMEPLAY_RUN_H
#define BFM_GAMEPLAY_RUN_H

#include "forest_status.h"
#include <stdint.h>

typedef struct BfmGameplayRun {
    /* The original startup clears this word once. Rooms share the stream. */
    uint32_t random_state;
    uint8_t story_flags[64];
    /* The ability, its HUD label and current BP travel with this playthrough. */
    BfmForestStatus player;
    int has_player;
} BfmGameplayRun;

uint16_t bfm_gameplay_random(BfmGameplayRun *run);
int bfm_gameplay_story_flag(const BfmGameplayRun *run, unsigned flag);

/* Commit the summit award once to the current playthrough. */
int bfm_gameplay_acquire_lumina(BfmGameplayRun *run);

#endif
