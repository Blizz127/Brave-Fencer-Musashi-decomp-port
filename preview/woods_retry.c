/* SPDX-License-Identifier: MIT */
#include "woods_retry.h"
#include "../pc_port/forest_player_archive.h"

BfmWoodsRetryPage bfm_woods_retry_begin(const BfmGameplayRun *run)
{
    if (!run) return BFM_WOODS_RETRY_INVALID;
    return run->memory_box.present || run->saved_data.present ?
           BFM_WOODS_RETRY_CHOOSE : BFM_WOODS_RESTART_CHAPTER;
}

BfmWoodsRetryPage bfm_woods_retry_answer(const BfmGameplayRun *run,
                                        BfmWoodsRetryPage page, unsigned answer)
{
    if (!run) return BFM_WOODS_RETRY_INVALID;
    if (page == BFM_WOODS_RETRY_MISSING)
        return answer == 0 ? BFM_WOODS_RETRY_CHOOSE : BFM_WOODS_RETRY_INVALID;
    if (answer != 1 && answer != 2) return BFM_WOODS_RETRY_INVALID;
    switch (page) {
    case BFM_WOODS_RETRY_CHOOSE:
        return answer == 1 ? BFM_WOODS_RETRY_BOX : BFM_WOODS_RETRY_SAVED;
    case BFM_WOODS_RETRY_BOX:
        if (answer == 2) return BFM_WOODS_RETRY_CHOOSE;
        return run->memory_box.present ? BFM_WOODS_RESTORE_BOX : BFM_WOODS_RETRY_MISSING;
    case BFM_WOODS_RETRY_SAVED:
        if (answer == 2) return BFM_WOODS_RETRY_CHOOSE;
        return run->saved_data.present ? BFM_WOODS_RESTORE_SAVED : BFM_WOODS_RETRY_MISSING;
    default:
        return BFM_WOODS_RETRY_INVALID;
    }
}

BfmForestMessage *bfm_woods_retry_window(const MusashiDiscMedia *disc, BfmWoodsRetryPage page)
{
    static const struct { uint32_t address; unsigned bytes; } window[] = {
        [BFM_WOODS_RETRY_CHOOSE] = {UINT32_C(0x80110c3c), 87},
        [BFM_WOODS_RETRY_BOX] = {UINT32_C(0x80110c94), 62},
        [BFM_WOODS_RETRY_SAVED] = {UINT32_C(0x80110cd4), 53},
        [BFM_WOODS_RETRY_MISSING] = {UINT32_C(0x80110d0c), 30}
    };
    ForestPlayerArchive mounted;
    uint8_t program[87];
    if ((unsigned)page >= sizeof window / sizeof *window ||
        !forest_player_archive_open(disc, &mounted) ||
        !forest_player_archive_read(&mounted, window[page].address, program, window[page].bytes))
        return NULL;
    return bfm_field_message_open(program, window[page].bytes);
}

int bfm_woods_retry_destination(const MusashiDiscMedia *disc, const BfmGameplayRun *run,
                                BfmWoodsRetryPage page, const uint8_t *woods_scene,
                                size_t scene_bytes, BfmBedroomEntry *out)
{
    BfmCheckpointKind kind;
    if (page == BFM_WOODS_RESTORE_BOX) kind = BFM_CHECKPOINT_MEMORY_BOX;
    else if (page == BFM_WOODS_RESTORE_SAVED) kind = BFM_CHECKPOINT_SAVED_DATA;
    else return 0;
    BfmSaveRecord restored;
    if (!out || !bfm_gameplay_woods_recovery(run, kind, woods_scene, scene_bytes, &restored))
        return 0;
    return bfm_woods_restart_entry(disc, &restored, out);
}
