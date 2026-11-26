/* SPDX-License-Identifier: MIT */
#include "save_menu.h"
#include "card_image.h"
#include "save_payload.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { SUMMARY_START = 640, SUMMARY_SIZE = 128, FIELDS_START = 16, MIRROR_START = 36 };

static unsigned little16(const uint8_t *p)
{
    return p[0] + 256u * p[1];
}

static unsigned sum_bytes(const uint8_t *p, size_t n)
{
    unsigned sum = 0;
    while (n--) sum += *p++;
    return sum & 65535u;
}

static int blank(const uint8_t *p, size_t n)
{
    while (n--) if (*p++) return 0;
    return 1;
}

static BfmSaveRow read_row(const uint8_t *summary, const uint8_t *payload)
{
    BfmSaveRow row = {0};
    const uint8_t *fields = summary + FIELDS_START;
    const size_t length = SUMMARY_SIZE - FIELDS_START;
    if (blank(summary, SUMMARY_SIZE) && blank(payload, MUSASHI_PAYLOAD_BYTES))
        return row;
    row.state = BFM_SAVE_DAMAGED;
    if (summary[0] != 255 || little16(summary + 2) != sum_bytes(fields, length) ||
        little16(summary + 4) != sum_bytes(payload, MUSASHI_PAYLOAD_BYTES) ||
        memcmp(fields, payload + MIRROR_START, length) != 0)
        return row;
    row.hours = fields[7];
    row.minutes = fields[6];
    row.day = little16(fields + 52);
    row.hour = fields[57];
    row.minute = fields[56];
    if (row.day == 0 || row.hour >= 24 || row.minute >= 60 || row.minutes >= 60)
        return row;
    /* This card lineage has original menu badges for these two story states. */
    unsigned story = little16(fields + 8);
    if (!little16(fields + 10))
        row.chapter = story == 2 ? 1u : story == 10 ? 2u : 0u;
    row.hp = little16(fields + 60);
    row.max_hp = little16(fields + 58);
    row.bp = little16(fields + 64);
    row.max_bp = little16(fields + 62);
    row.rescued = little16(fields + 12);
    row.money = ((uint64_t)little16(fields + 22) * 65536u + little16(fields + 20)) * 10u;
    row.state = BFM_SAVE_VALID;
    return row;
}

static void read_directory(BfmSaveMenu *menu, CardImage image)
{
    static const char name[] = "BASLUS-00726MUSASHI";
    CardEntry entry;
    CardResult result = card_open(image, name, &entry);
    if (result == CARD_NOT_FOUND) return;
    uint8_t game[CARD_BLOCK_SIZE];
    size_t size;
    if (result != CARD_OK || entry.size != sizeof game ||
        card_read(image, name, 0, game, sizeof game, &size) != CARD_OK ||
        size != sizeof game || game[0] != 'S' || game[1] != 'C' || game[3] != 1) {
        menu->issue = "Unreadable memory card or game file.";
        return;
    }
    if (game[512] < MUSASHI_SAVE_FILES) menu->selected = game[512];
    for (unsigned i = 0; i < MUSASHI_SAVE_FILES; ++i) {
        const uint8_t *payload = game + musashi_save_payload_offset(i + 1);
        menu->rows[i] = read_row(game + SUMMARY_START + i * SUMMARY_SIZE, payload);
        if (menu->rows[i].state == BFM_SAVE_VALID)
            memcpy(&menu->records[i], payload, sizeof menu->records[i]);
    }
}

void bfm_save_menu_open(BfmSaveMenu *menu, const char *path)
{
    *menu = (BfmSaveMenu){0};
    if (!path) {
        menu->issue = "Specify a raw memory card with --card.";
        return;
    }
    FILE *file = fopen(path, "rb");
    if (!file) {
        menu->issue = "Could not open the memory card.";
        return;
    }
    uint8_t *bytes = malloc(CARD_IMAGE_SIZE);
    if (!bytes) menu->issue = "Insufficient memory to read the card.";
    else if (fread(bytes, 1, CARD_IMAGE_SIZE, file) != CARD_IMAGE_SIZE ||
             fgetc(file) != EOF || ferror(file))
        menu->issue = "Expected a raw 128 KiB memory card.";
    else
        read_directory(menu, (CardImage){bytes, CARD_IMAGE_SIZE});
    free(bytes);
    fclose(file);
}

int bfm_save_menu_input(BfmSaveMenu *menu, uint16_t pressed)
{
    enum { START = 1u << 3, UP = 1u << 4, RIGHT = 1u << 5, DOWN = 1u << 6, LEFT = 1u << 7,
           TRIANGLE = 1u << 12, CIRCLE = 1u << 13, CROSS = 1u << 14 };
    if (menu->issue) return (pressed & (START | TRIANGLE | CIRCLE | CROSS)) != 0;
    if (menu->page != BFM_SAVE_PICK) {
        if (pressed & TRIANGLE) menu->page = BFM_SAVE_PICK;
        else if ((pressed & CROSS) && menu->page == BFM_SAVE_CONFIRM)
            menu->page = BFM_SAVE_BOUNDARY;
        return 0;
    }
    if (pressed & TRIANGLE) return 1;
    if ((pressed & (LEFT | RIGHT)) == LEFT || (pressed & (LEFT | RIGHT)) == RIGHT)
        menu->selected ^= 1u;
    if ((pressed & (UP | DOWN)) == UP || (pressed & (UP | DOWN)) == DOWN)
        menu->selected ^= 2u;
    if ((pressed & CROSS) && menu->rows[menu->selected].state == BFM_SAVE_VALID)
        menu->page = BFM_SAVE_CONFIRM;
    return 0;
}
