/* SPDX-License-Identifier: MIT */
#include "inn_card.h"
#include "save_payload.h"

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

const char *bfm_inn_card_encode(CardImage image, unsigned file, const BfmSaveRecord *record)
{
    if (!record || file < 1 || file > MUSASHI_SAVE_FILES)
        return "Choose a save file.";

    /* Only the story-ten inn record has a confirmed exterior restore. */
    const uint8_t *p = record->progress;
    if (p[8] != 10 || p[9] || p[10] || p[11] || !bfm_save_uses_default_room_table(record))
        return "This story cannot be saved at the inn.";
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
    if (!empty_bytes(summary, SUMMARY_BYTES) || !empty_bytes(payload, MUSASHI_PAYLOAD_BYTES))
        return "Choose an empty save file.";

    /* The inn records progress and its four state banks. Its destination and
     * trailing reserved bytes stay zero: Continue selects the exterior. */
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

const char *bfm_inn_card_save(const uint8_t *expected, unsigned file,
                              const BfmSaveRecord *record, BfmInnCardCommit commit,
                              void *context, const MusashiDiscMedia *disc)
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
        issue = bfm_inn_card_encode(image, file, record);
    if (!issue && !commit(context, 0, expected, replacement))
        issue = "Could not commit this save. Reopen the card and try again.";
    free(replacement);
    return issue;
}
