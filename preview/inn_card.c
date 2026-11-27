/* SPDX-License-Identifier: MIT */
#include "inn_card.h"
#include "save_payload.h"
#include "save_menu.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>
#include <string.h>

enum {
    LAST_SELECTED = 512,
    SUMMARY_START = 640,
    SUMMARY_BYTES = 128,
    DISPLAY_START = 16
};

/* All fields of the on-card live record are byte arrays. */
typedef char InnRecordSize[sizeof(BfmSaveRecord) == 732 ? 1 : -1];

typedef enum { HOTELO, HOTELO_OVERWRITE, CHAPTER_END } SavePlace;

static const char name[] = "BASLUS-00726MUSASHI";

static int empty_bytes(const uint8_t *bytes, size_t size)
{
    for (size_t i = 0; i < size; ++i)
        if (bytes[i]) return 0;
    return 1;
}

static void store_sum(uint8_t *result, const uint8_t *bytes, size_t size)
{
    unsigned sum = 0;
    for (size_t i = 0; i < size; ++i) sum += bytes[i];
    result[0] = (uint8_t)sum;
    result[1] = (uint8_t)(sum >> 8);
}

static const char *encode_file(CardImage image, unsigned file, const BfmSaveRecord *record,
                               SavePlace place)
{
    if (!record || file < 1 || file > MUSASHI_SAVE_FILES)
        return "Choose a save file.";

    const uint8_t *p = record->progress;
    uint32_t story = u32le(p + 8);
    int supported_story = place == CHAPTER_END ? story == 2 : bfm_story_returns_to_grillin_inn(story);
    if (!supported_story || !bfm_save_uses_default_room_table(record))
        return place == CHAPTER_END ? "Invalid chapter save." : "This story cannot be saved at the inn.";
    if (place == CHAPTER_END && (!(record->flags[16] & 1u) ||
                                !empty_bytes(record->destination, sizeof record->destination)))
        return "Invalid chapter destination or sword state.";
    if (!(p[52] || p[53]) || p[6] >= 60 || p[56] >= 60 || p[57] >= 24)
        return "Invalid play time or game clock.";

    CardEntry entry;
    CardResult result = card_open(image, name, &entry);
    if (result == CARD_NOT_FOUND) return "This card needs a Musashi game file.";
    uint8_t game[CARD_BLOCK_SIZE];
    size_t length;
    if (result != CARD_OK || entry.size != sizeof game ||
        card_read(image, name, 0, game, sizeof game, &length) != CARD_OK ||
        length != sizeof game || game[0] != 'S' || game[1] != 'C' || game[3] != 1)
        return "Could not read the Musashi game file.";

    uint8_t *summary = game + SUMMARY_START + (file - 1) * SUMMARY_BYTES;
    uint8_t *payload = game + musashi_save_payload_offset(file);
    if (place == HOTELO_OVERWRITE) {
        BfmSaveMenu card;
        bfm_save_menu_read_image(&card, image.bytes, image.length);
        if (card.issue || card.rows[file - 1].state != BFM_SAVE_VALID)
            return "The selected file cannot be overwritten.";
        /* Each overwritten sector is rebuilt. Old padding is not part of the new save. */
        memset(game + LAST_SELECTED, 0, SUMMARY_START - LAST_SELECTED);
        memset(summary, 0, SUMMARY_BYTES);
        memset(payload, 0, MUSASHI_PAYLOAD_BYTES);
    } else if (!empty_bytes(summary, SUMMARY_BYTES) || !empty_bytes(payload, MUSASHI_PAYLOAD_BYTES)) {
        return "Choose an empty save file.";
    }

    /* The keeper and chapter-end saves resume at a resident destination. */
    BfmSaveRecord saved = *record;
    memset(saved.destination, 0, sizeof saved.destination);
    memcpy(payload, &saved, sizeof saved);
    memcpy(summary + DISPLAY_START, saved.progress, SUMMARY_BYTES - DISPLAY_START);
    summary[0] = 255;
    store_sum(summary + 2, summary + DISPLAY_START, SUMMARY_BYTES - DISPLAY_START);
    store_sum(summary + 4, payload, MUSASHI_PAYLOAD_BYTES);
    game[LAST_SELECTED] = (uint8_t)(file - 1);

    if (card_write(image, name, 0, game, sizeof game) != CARD_OK)
        return "Could not update the Musashi game file.";
    return NULL;
}

const char *bfm_inn_card_encode(CardImage image, unsigned file, const BfmSaveRecord *record)
{
    return encode_file(image, file, record, HOTELO);
}

/* The mounted US disc supplies the title and three 16x16 four-bit TIMs.
 * Each image uses the same palette. The remaining header fields are zero. */
static int create_game_file(CardImage image, const MusashiDiscMedia *disc)
{
    enum { TIM_START = 0x542d8, TIM_BYTES = 192, FRAME_BYTES = 128 };
    uint8_t header[512] = {'S', 'C', 0x13, 1};
    size_t size;
    if (!disc || !musashi_disc_media_read_root_range(disc, "SLUS_007.26", 0x63610,
                                header + 4, 40, &size) ||
        !musashi_disc_media_read_root_range(disc, "SLUS_007.26", TIM_START + 20,
                                header + 96, 32, &size)) return 0;
    for (unsigned i = 0; i < 3; ++i) {
        size_t pixels = TIM_START + i * TIM_BYTES + 64;
        if (!musashi_disc_media_read_root_range(disc, "SLUS_007.26", pixels,
                        header + 128 + i * FRAME_BYTES, FRAME_BYTES, &size)) return 0;
    }
    return card_create(image, name, 1) == CARD_OK &&
           card_write(image, name, 0, header, sizeof header) == CARD_OK;
}

static const char *save_file(const uint8_t *expected, unsigned file,
                              const BfmSaveRecord *record, BfmInnCardCommit commit,
                              void *context, const MusashiDiscMedia *disc, SavePlace place)
{
    if (!expected || !commit) return "Select a writable card copy to save.";
    uint8_t *replacement = malloc(CARD_IMAGE_SIZE);
    if (!replacement) return "Insufficient memory to save.";
    memcpy(replacement, expected, CARD_IMAGE_SIZE);
    CardImage image = {replacement, CARD_IMAGE_SIZE};
    const char *issue;
    if (card_is_unallocated(image) && !create_game_file(image, disc))
        issue = "Could not create the Musashi game file from your disc.";
    else
        issue = encode_file(image, file, record, place);
    if (!issue && !commit(context, 0, expected, replacement))
        issue = "Could not commit this save. Reopen the card and try again.";
    free(replacement);
    return issue;
}

const char *bfm_inn_card_save(const uint8_t *expected, unsigned file,
                              const BfmSaveRecord *record, BfmInnCardCommit commit,
                              void *context, const MusashiDiscMedia *disc)
{
    return save_file(expected, file, record, commit, context, disc, HOTELO);
}

const char *bfm_inn_card_overwrite(const uint8_t *expected, unsigned file,
                                   const BfmSaveRecord *record, BfmInnCardCommit commit,
                                   void *context, const MusashiDiscMedia *disc)
{
    return save_file(expected, file, record, commit, context, disc, HOTELO_OVERWRITE);
}

const char *bfm_chapter_card_save(const uint8_t *expected, unsigned file,
                                 const BfmSaveRecord *record, BfmInnCardCommit commit,
                                 void *context, const MusashiDiscMedia *disc)
{
    return save_file(expected, file, record, commit, context, disc, CHAPTER_END);
}
