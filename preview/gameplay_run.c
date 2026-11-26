/* SPDX-License-Identifier: MIT */
#include "gameplay_run.h"
#include "../pc_port/forest_player_archive.h"
#include <string.h>

enum { FIGURE_CHAPTER = 0x2e, FIRST_FIGURE = 0x63 };

int bfm_gameplay_new_player(BfmGameplayRun *run, const MusashiDiscMedia *disc)
{
    enum { OFFER_COUNT = 6, ROW_SIZE = 8 };
    const uint32_t first_offer = UINT32_C(0x8010f46c) + ROW_SIZE;
    ForestPlayerArchive archive;
    uint8_t offers[OFFER_COUNT * ROW_SIZE];
    BfmForestStatus player;
    if (!run || !bfm_forest_status_load(disc, &player) || !forest_player_archive_open(disc, &archive) ||
        !forest_player_archive_read(&archive, first_offer, offers, sizeof offers)) return 0;

    BfmGameplayBanks fresh = {0};
    fresh.variables[0x2c] = fresh.variables[0x2d] = 7;
    fresh.variables[FIGURE_CHAPTER] = 1;
    memset(fresh.variables + 0x3b, 3, 5);
    for (unsigned i = 0; i < OFFER_COUNT; ++i) {
        const uint8_t *offer = offers + ROW_SIZE * i;
        memcpy(fresh.figure_prices + 2 * i, offer, 2);
        fresh.variables[FIRST_FIGURE + i] = offer[2] | 0x80u;
    }
    run->player = player;
    run->banks = fresh;
    run->has_player = 1;
    return 1;
}

int bfm_gameplay_advance_figures(BfmGameplayRun *run)
{
    if (!run || !run->has_player || run->banks.variables[FIGURE_CHAPTER] != 1) return 0;
    run->banks.variables[FIGURE_CHAPTER] = 2;
    return 1;
}

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

void bfm_gameplay_advance_playtime(uint8_t progress[152], unsigned updates)
{
    uint8_t *time = progress + 4;
    while (updates--) {
        if (++time[0] < 30) continue;
        time[0] = 0;
        if (++time[1] < 60) continue;
        time[1] = 0;
        if (++time[2] < 60) continue;
        time[2] = 0;
        if (++time[3] >= 100) {
            time[1] = 59;
            time[2] = 59;
            time[3] = 99;
        }
    }
}
