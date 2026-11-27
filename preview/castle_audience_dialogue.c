/* SPDX-License-Identifier: MIT */
#include "castle_audience_dialogue.h"
#include "../pc_port/forest_player_bytes.h"

enum {
    ARRIVING, INTRODUCTION, ANNOUNCE_HELPERS, NAME_HELPERS, PARTIAL_ROSTER,
    SEARCH_AGAIN, FULL_ROSTER, OPTIONAL_VISIT, DEPARTING
};

/* Offsets of authored message rows in the mounted SC01/9 court program.
 * Each message row contains the speaker and a pointer to the player's
 * localized text stream. Control rows between them belong to the court. */
enum {
    FIRST_QUESTION = 0xc1188, RETURN_QUESTION = 0xc1270,
    ROSTER_HEADING = 0xc11b8, FIRST_HELPER_NAME = 0xc11d0,
    NONE_RESCUED = 0xc1240, PARTIAL_END = 0xc1250,
    FOREST_SEARCH = 0xc1258, PARTY_READY = 0xc1130
};

int bfm_castle_audience_eligible(const BfmSaveRecord *save)
{
    if (!save) return 0;
    uint32_t story = u32le(save->progress + 8);
    return story >= 135 && story < 200 && save->variables[59] <= 4;
}

void bfm_ribson_dialogue_begin(BfmRibsonDialogue *talk, const uint8_t *scene, size_t bytes)
{
    /* Sunday File 4: first live 3003 field to Ribson's selector is 51
     * field updates. V5099 and V5201 include both ends of that interval. */
    *talk = (BfmRibsonDialogue){.scene=scene, .bytes=bytes, .wait=50};
}

static BfmRibsonResult open_row(BfmRibsonDialogue *talk, size_t row, BfmForestMessage **out)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (!talk->scene || row > talk->bytes || talk->bytes - row < 8)
        return BFM_RIBSON_INVALID;
    unsigned speaker = u32le(talk->scene + row);
    uint32_t text = u32le(talk->scene + row + 4);
    if (speaker > 1 || text < base || text - base >= talk->bytes)
        return BFM_RIBSON_INVALID;
    size_t offset = text - base;
    *out = bfm_field_message_borrow(talk->scene + offset, talk->bytes - offset);
    if (!*out) return BFM_RIBSON_INVALID;
    talk->hero = (int)speaker;
    return BFM_RIBSON_PAGE;
}

static void publish_progress(BfmSaveRecord *save, BfmGameplayRun *run,
                             uint8_t story, uint8_t audience)
{
    save->progress[8] = story;
    save->progress[9] = save->progress[10] = save->progress[11] = 0;
    save->variables[59] = audience;
    if (run) {
        run->banks.variables[59] = audience;
        if (run->has_player) {
            run->player.record[8] = story;
            run->player.record[9] = run->player.record[10] = run->player.record[11] = 0;
        }
    }
}

BfmRibsonResult bfm_ribson_dialogue_step(BfmRibsonDialogue *talk, BfmSaveRecord *save,
                                        BfmGameplayRun *run, BfmForestMessage **out)
{
    if (!talk || !save || !out) return BFM_RIBSON_INVALID;
    *out = NULL;
    if (talk->wait) {
        --talk->wait;
        return BFM_RIBSON_WAIT;
    }

    if (talk->phase == NAME_HELPERS) {
        while (talk->row < 4) {
            unsigned slot = talk->row++;
            if (save->variables[69 + slot]) {
                ++talk->helpers;
                return open_row(talk,FIRST_HELPER_NAME + 24 * slot,out);
            }
        }
        talk->row = 0;
        talk->phase = talk->helpers == 4 ? FULL_ROSTER : PARTIAL_ROSTER;
    }

    switch (talk->phase) {
    case ARRIVING: {
        if (!bfm_castle_audience_eligible(save)) return BFM_RIBSON_INVALID;
        int first = save->variables[59] < 4;
        BfmRibsonResult result = open_row(talk,first ? FIRST_QUESTION : RETURN_QUESTION,out);
        if (result != BFM_RIBSON_PAGE) return result;
        if (first) publish_progress(save,run,140,4);
        talk->phase = first ? INTRODUCTION : ANNOUNCE_HELPERS;
        talk->row = 1;
        return result;
    }
    case INTRODUCTION: {
        size_t text = FIRST_QUESTION + 8 * talk->row;
        if (++talk->row == 6) talk->phase = ANNOUNCE_HELPERS;
        return open_row(talk,text,out);
    }
    case ANNOUNCE_HELPERS:
        talk->phase = NAME_HELPERS;
        talk->row = 0;
        return open_row(talk,ROSTER_HEADING,out);
    case PARTIAL_ROSTER:
        talk->phase = SEARCH_AGAIN;
        return open_row(talk,talk->helpers ? PARTIAL_END : NONE_RESCUED,out);
    case SEARCH_AGAIN: {
        size_t text = FOREST_SEARCH + 8 * talk->row;
        if (++talk->row == 2) talk->phase = OPTIONAL_VISIT;
        return open_row(talk,text,out);
    }
    case FULL_ROSTER:
        if (talk->row < 7)
            return open_row(talk,PARTY_READY + 8 * talk->row++,out);
        if (talk->row == 7) {
            BfmRibsonResult result = open_row(talk,PARTY_READY + 8 * 8,out);
            if (result != BFM_RIBSON_PAGE) return result;
            publish_progress(save,run,150,5);
            talk->phase = DEPARTING;
            return result;
        }
        return BFM_RIBSON_INVALID;
    case OPTIONAL_VISIT:
        return BFM_RIBSON_SERVICE;
    case DEPARTING:
        return BFM_RIBSON_DEPART;
    default:
        return BFM_RIBSON_INVALID;
    }
}
