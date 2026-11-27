/* SPDX-License-Identifier: MIT */
#include "chapter_restart.h"
#include "bedroom_score.h"
#include "../pc_port/forest_player_archive.h"
#include <string.h>

int bfm_chapter_restart_record(const MusashiDiscMedia *disc, const BfmGameplayRun *run,
                               BfmSaveRecord *out)
{
    if (!disc || !run || !run->has_player || !out ||
        run->saved_data.present || run->memory_box.present) return 0;

    BfmSaveRecord record = {0};
    ForestPlayerArchive archive;
    if (!forest_player_archive_open(disc, &archive) ||
        !forest_player_archive_read(&archive, UINT32_C(0x800d3a44), record.destination,
                                    sizeof record.destination) ||
        !bfm_bedroom_resident_data(disc, UINT32_C(0x80072c84), record.progress,
                                  sizeof record.progress)) return 0;
    /* This path restores the resident template, including its stored clock
     * display. The castle wake performs the next resource/clock updates. */
    memcpy(record.progress + 4, run->player.record + 4, 4);
    record.progress[8] = 2;
    memset(record.progress + 9, 0, 3);
    record.progress[0x48] |= 1;
    record.flags[1] = run->story_flags[1] & 0x38u;
    record.flags[16] = 1;
    BfmGameplayBanks banks = {0};
    memcpy(banks.figure_prices, run->banks.figure_prices, sizeof banks.figure_prices);
    memcpy(banks.variables + 44, run->banks.variables + 44, 2);
    if (!bfm_gameplay_seed_figures(&banks, disc)) return 0;
    memcpy(record.figure_prices, banks.figure_prices, sizeof record.figure_prices);
    memcpy(record.variables, banks.variables, sizeof record.variables);
    *out = record;
    return 1;
}

BfmBedroom *bfm_chapter_restart_open(const MusashiDiscMedia *disc, BfmGameplayRun *run)
{
    BfmSaveRecord record;
    if (!bfm_chapter_restart_record(disc, run, &record)) return NULL;
    BfmBedroom *room = bfm_bedroom_open_restart(disc, &record);
    if (!room) return NULL;
    BfmGameplayRun staged = *run;
    bfm_bedroom_bind_run(room, &staged);
    if (!bfm_bedroom_step(room)) {
        bfm_bedroom_free(room);
        return NULL;
    }
    *run = staged;
    bfm_bedroom_bind_run(room, run);
    return room;
}
