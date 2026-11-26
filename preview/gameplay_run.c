/* SPDX-License-Identifier: MIT */
#include "gameplay_run.h"

uint16_t bfm_gameplay_random(BfmGameplayRun *run)
{
    run->random_state = run->random_state * UINT32_C(1103515245) + UINT32_C(12345);
    return (uint16_t)((run->random_state >> 16) & 32767u);
}

int bfm_gameplay_story_flag(const BfmGameplayRun *run, unsigned flag)
{
    return run && flag < sizeof run->story_flags * 8u &&
           (run->story_flags[flag >> 3] & (1u << (flag & 7u))) != 0;
}

int bfm_gameplay_acquire_lumina(BfmGameplayRun *run)
{
    enum { lumina = 0x80 };
    if (!run || !run->has_player) return 0;
    if (!bfm_gameplay_story_flag(run, lumina)) {
        bfm_forest_status_equip_lumina(&run->player);
        run->story_flags[lumina >> 3] |= 1u << (lumina & 7u);
    }
    return 1;
}
