/* SPDX-License-Identifier: MIT */
#include "chapter_completion.h"
#include <string.h>

enum {
    RIGHT = 1u << 5, LEFT = 1u << 7, UP = 1u << 4, DOWN = 1u << 6,
    CROSS = 1u << 14, TRIANGLE = 1u << 12,
    /* The first two boundaries fall between consecutive 30-field
     * observations of the card UI. The resume prompt has its own input. */
    CARD_READ_FIELDS = 256, CARD_WRITE_FIELDS = 240, SAVED_FIELDS = 156,
    TITLE_TO_BEDROOM_FIELDS = 584
};

static void page(BfmChapterCompletion *state, BfmChapterSavePage next)
{
    state->page = next;
    state->fields = 0;
}

int bfm_chapter_completion_begin(BfmChapterCompletion *state, BfmGameplayRun *run,
                                 const BfmSaveRecord *record)
{
    if (!state || state->page != BFM_CHAPTER_INACTIVE || !run || !run->has_player || !record ||
        memcmp(record->progress + 8, (uint8_t[]){2, 0, 0, 0}, 4) ||
        memcmp(record->destination, (uint8_t[36]){0}, sizeof record->destination) ||
        !bfm_save_uses_default_room_table(record) || !bfm_gameplay_story_flag(run, 128) ||
        memcmp(record->flags, run->story_flags, sizeof record->flags) ||
        memcmp(record->progress, run->player.record, sizeof record->progress)) return 0;
    state->earned = *record;
    state->run = run;
    page(state, BFM_CHAPTER_SAVE_PROMPT);
    return 1;
}

int bfm_chapter_completion_begin_earned(BfmChapterCompletion *state, BfmGameplayRun *run)
{
    if (!run || !run->has_player || run->banks.variables[0x2e] != 2 ||
        memcmp(run->player.record + 8, (uint8_t[4]){0}, 4)) return 0;

    /* An unset saved destination lets Continue select the castle bedroom.
     * The active field's transfer descriptor is a different record. */
    BfmSaveRecord record = {0};
    memcpy(record.figure_prices, run->banks.figure_prices, sizeof record.figure_prices);
    memcpy(record.variables, run->banks.variables, sizeof record.variables);
    memcpy(record.timestamps, run->banks.timestamps, sizeof record.timestamps);
    memcpy(record.flags, run->story_flags, sizeof record.flags);
    run->player.record[8] = 2;
    memcpy(record.progress, run->player.record, sizeof record.progress);
    if (bfm_chapter_completion_begin(state, run, &record)) return 1;
    run->player.record[8] = 0;
    return 0;
}

static void fail(BfmChapterCompletion *state, const char *issue)
{
    state->issue = issue;
    page(state, BFM_CHAPTER_SAVE_ERROR);
}

static void read_card(BfmChapterCompletion *state, BfmInnProfile *copy)
{
    if (!copy) {
        fail(state, "Open a writable card copy to save.");
        return;
    }
    bfm_inn_profile_menu(copy, &state->files);
    if (state->files.issue || state->files.selected >= 4 ||
        (state->files.page != BFM_SAVE_PICK && state->files.page != BFM_SAVE_CARD_LOADING)) {
        fail(state, "Could not read the memory card.");
        return;
    }
    page(state, BFM_CHAPTER_SAVE_READING);
}

static void finish_write(BfmChapterCompletion *state, BfmInnProfile *copy,
                         const MusashiDiscMedia *disc)
{
    unsigned selected = state->files.selected;
    const char *issue = bfm_chapter_profile_save(copy, selected + 1, &state->earned, disc);
    if (issue) {
        fail(state, issue);
        return;
    }
    bfm_inn_profile_menu(copy, &state->files);
    if (state->files.issue || state->files.page != BFM_SAVE_PICK ||
        state->files.selected != selected || state->files.rows[selected].state != BFM_SAVE_VALID ||
        memcmp(&state->files.records[selected], &state->earned, sizeof state->earned)) {
        fail(state, "Could not verify the saved file.");
        return;
    }
    page(state, BFM_CHAPTER_SAVED);
}

int bfm_chapter_completion_step(BfmChapterCompletion *state, const MusashiDiscMedia *disc,
                                BfmInnProfile *copy, uint16_t pressed)
{
    if (!state || !state->run) return 0;
    if (state->fields < UINT32_MAX) ++state->fields;
    switch (state->page) {
    case BFM_CHAPTER_SAVE_PROMPT:
        if (pressed & TRIANGLE) page(state, BFM_CHAPTER_RESUME);
        else if (pressed & CROSS) read_card(state, copy);
        break;
    case BFM_CHAPTER_SAVE_READING:
        if (state->fields >= CARD_READ_FIELDS) page(state, BFM_CHAPTER_SAVE_FILES);
        break;
    case BFM_CHAPTER_SAVE_FILES: {
        if (pressed & TRIANGLE) return 0;
        unsigned horizontal = pressed & (RIGHT | LEFT), vertical = pressed & (UP | DOWN);
        if (horizontal == RIGHT || horizontal == LEFT) state->files.selected ^= 1u;
        if (vertical == UP || vertical == DOWN) state->files.selected ^= 2u;
        if (pressed & CROSS) {
            if (state->files.rows[state->files.selected].state == BFM_SAVE_EMPTY)
                page(state, BFM_CHAPTER_SAVE_CONFIRM);
            else fail(state, "Choose an empty file.");
        }
        break;
    }
    case BFM_CHAPTER_SAVE_CONFIRM:
        if (pressed & TRIANGLE) return 0;
        if (pressed & CROSS) page(state, BFM_CHAPTER_SAVE_WRITING);
        break;
    case BFM_CHAPTER_SAVE_WRITING:
        if (state->fields >= CARD_WRITE_FIELDS) finish_write(state, copy, disc);
        break;
    case BFM_CHAPTER_SAVED:
        if (state->fields >= SAVED_FIELDS) page(state, BFM_CHAPTER_RESUME);
        break;
    case BFM_CHAPTER_RESUME:
        if (pressed & TRIANGLE) return 0;
        if (pressed & CROSS) page(state, BFM_CHAPTER_TWO_TITLE);
        break;
    case BFM_CHAPTER_TWO_TITLE:
        if (state->fields >= TITLE_TO_BEDROOM_FIELDS) page(state, BFM_CHAPTER_BEDROOM);
        break;
    case BFM_CHAPTER_SAVE_ERROR:
        if (pressed & CROSS) {
            state->issue = NULL;
            page(state, BFM_CHAPTER_SAVE_PROMPT);
        }
        break;
    case BFM_CHAPTER_BEDROOM:
        break;
    default:
        return 0;
    }
    return 1;
}

BfmBedroom *bfm_chapter_completion_take_bedroom(BfmChapterCompletion *state,
                                                 const MusashiDiscMedia *disc)
{
    if (!state || state->page != BFM_CHAPTER_BEDROOM || !disc) return NULL;
    BfmBedroom *room = bfm_bedroom_open_chapter(disc, &state->earned, state->run);
    if (room) page(state, BFM_CHAPTER_COMPLETION_FINISHED);
    return room;
}
