/* SPDX-License-Identifier: MIT */
#ifndef BFM_GAMEPLAY_RUN_H
#define BFM_GAMEPLAY_RUN_H

#include <stdint.h>

typedef struct BfmGameplayRun {
    /* The original startup clears this word once. Rooms share the stream. */
    uint32_t random_state;
    uint8_t story_flags[64];
} BfmGameplayRun;

uint16_t bfm_gameplay_random(BfmGameplayRun *run);
int bfm_gameplay_story_flag(const BfmGameplayRun *run, unsigned flag);

#endif
