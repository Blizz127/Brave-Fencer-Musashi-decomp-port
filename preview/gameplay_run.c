/* SPDX-License-Identifier: MIT */
#include "gameplay_run.h"
#include "../pc_port/forest_player_archive.h"
#include <string.h>

enum { FIGURE_CHAPTER = 0x2e, FIRST_FIGURE = 0x63 };

int bfm_gameplay_seed_figures(BfmGameplayBanks *banks, const MusashiDiscMedia *disc)
{
    enum { OFFER_COUNT = 6, ROW_SIZE = 8 };
    const uint32_t first_offer = UINT32_C(0x8010f46c) + ROW_SIZE;
    ForestPlayerArchive archive;
    uint8_t offers[OFFER_COUNT * ROW_SIZE];
    if (!banks || !forest_player_archive_open(disc, &archive) ||
        !forest_player_archive_read(&archive, first_offer, offers, sizeof offers)) return 0;

    banks->variables[FIGURE_CHAPTER] = 1;
    memset(banks->variables + 0x3b, 3, 5);
    for (unsigned i = 0; i < OFFER_COUNT; ++i) {
        const uint8_t *offer = offers + ROW_SIZE * i;
        memcpy(banks->figure_prices + 2 * i, offer, 2);
        banks->variables[FIRST_FIGURE + i] = offer[2] | 0x80u;
    }
    return 1;
}

int bfm_gameplay_new_player(BfmGameplayRun *run, const MusashiDiscMedia *disc)
{
    BfmForestStatus player;
    BfmGameplayBanks fresh = {0};
    if (!run || !bfm_forest_status_load(disc, &player) || !bfm_gameplay_seed_figures(&fresh, disc))
        return 0;
    fresh.variables[0x2c] = fresh.variables[0x2d] = 7;
    run->player = player;
    run->banks = fresh;
    run->has_player = 1;
    return 1;
}

void bfm_gameplay_remember_saved(BfmGameplayRun *run, const BfmSaveRecord *record)
{
    if (!run || !record) return;
    run->saved_data.state = *record;
    memset(run->saved_data.state.destination, 0, sizeof run->saved_data.state.destination);
    run->saved_data.present = 1;
}

int bfm_gameplay_remember_box(BfmGameplayRun *run, const BfmSaveRecord *record,
                              const uint8_t *box_destination, size_t bytes)
{
    if (!run || !record || !box_destination || bytes < sizeof record->destination ||
        (!box_destination[0] && !box_destination[1])) return 0;
    BfmSaveRecord box = *record;
    memcpy(box.destination, box_destination, sizeof box.destination);
    run->memory_box.state = box;
    run->memory_box.present = 1;
    return 1;
}

int bfm_gameplay_has_checkpoint(const BfmGameplayRun *run, BfmCheckpointKind kind)
{
    if (!run) return 0;
    if (kind == BFM_CHECKPOINT_SAVED_DATA) return run->saved_data.present;
    if (kind == BFM_CHECKPOINT_MEMORY_BOX) return run->memory_box.present;
    return 0;
}

int bfm_gameplay_field_recovery(const BfmGameplayRun *run, BfmCheckpointKind kind,
                                const uint8_t *scene, size_t scene_bytes,
                                size_t saved_destination_offset, BfmSaveRecord *out)
{
    if (!out || !bfm_gameplay_has_checkpoint(run, kind)) return 0;
    BfmSaveRecord restored;
    if (kind == BFM_CHECKPOINT_MEMORY_BOX) {
        if (!run->has_player) return 0;
        restored = run->memory_box.state;
        const uint8_t *wallet = run->player.record + 0x14;
        uint32_t units = (uint32_t)wallet[0] | (uint32_t)wallet[1] << 8 |
                         (uint32_t)wallet[2] << 16 | (uint32_t)wallet[3] << 24;
        int64_t signed_units = units;
        if (units > INT32_MAX) signed_units -= INT64_C(0x100000000);
        uint32_t half = (uint32_t)(signed_units / 2);
        for (unsigned i = 0; i < 4; ++i)
            restored.progress[0x14 + i] = (uint8_t)(half >> (8 * i));
    } else {
        restored = run->saved_data.state;
        const uint8_t *story = restored.progress + 8;
        if (story[0] < 10 || story[0] >= 200 || story[1] || story[2] || story[3] ||
            !scene || saved_destination_offset > scene_bytes ||
            scene_bytes - saved_destination_offset < sizeof restored.destination) return 0;
        const uint8_t *destination = scene + saved_destination_offset;
        if (destination[0] != 0x12 || destination[1] != 0x30 ||
            destination[2] != 0 || destination[3] != 0) return 0;
        memcpy(restored.destination, destination, sizeof restored.destination);
    }
    *out = restored;
    return 1;
}

int bfm_gameplay_woods_recovery(const BfmGameplayRun *run, BfmCheckpointKind kind,
                                const uint8_t *scene, size_t scene_bytes, BfmSaveRecord *out)
{
    return bfm_gameplay_field_recovery(run, kind, scene, scene_bytes, BFM_CLEFT_RECOVERY_OFFSET, out);
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
