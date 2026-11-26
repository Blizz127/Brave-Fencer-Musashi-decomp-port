/* SPDX-License-Identifier: MIT */
#include "bedroom_record.h"
#include <stdlib.h>
#include <string.h>

enum { SECTOR = 2048, MAIN_BYTES_LIMIT = 1024 * 1024 };

static uint32_t word(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static int descriptor(const MusashiDiscMedia *disc, const char *name, unsigned member,
                      size_t *start, size_t *length)
{
    uint8_t row[8], count[4];
    size_t total;
    if (!musashi_disc_media_read_root_range(disc, name, 0, count, sizeof count, &total) ||
        member >= word(count) ||
        !musashi_disc_media_read_root_range(disc, name, 8u + 8u * member, row, sizeof row, &total))
        return 0;
    uint32_t sector = word(row);
    if (sector == 0 || sector > total / SECTOR) return 0;
    *start = (size_t)sector * SECTOR;
    *length = word(row + 4);
    return *length && *length <= total - *start;
}

/* In MAIN members 10 and 12 the sparse tile upload precedes the resident
 * data block. PAC block lengths include their header and are sector-aligned
 * on disc; the payload length in the header is unrounded. */
static int payload(const MusashiDiscMedia *disc, const char *archive, unsigned member, unsigned type,
                   size_t *position, size_t *bytes)
{
    size_t start, left, total;
    if (!descriptor(disc, archive, member, &start, &left)) return 0;
    while (left >= SECTOR) {
        uint8_t header[16];
        if (!musashi_disc_media_read_root_range(disc, archive, start, header,
                                                 sizeof header, &total) ||
            memcmp(header, "PAC", 4)) return 0;
        uint32_t length = word(header + 12);
        if (length <= SECTOR || length > left) return 0;
        if (word(header + 4) == type) {
            *position = start + SECTOR;
            *bytes = length - SECTOR;
            return *bytes <= MAIN_BYTES_LIMIT;
        }
        size_t sectors = (length - 1u) / SECTOR + 1u;
        if (sectors > left / SECTOR) return 0;
        start += sectors * SECTOR;
        left -= sectors * SECTOR;
    }
    return 0;
}

static uint8_t *read_payload(const MusashiDiscMedia *disc, const char *archive, unsigned member, unsigned type, size_t *size)
{
    size_t start, length, total;
    *size = 0;
    if (!payload(disc, archive, member, type, &start, &length)) return NULL;
    uint8_t *data = malloc(length);
    if (!data || !musashi_disc_media_read_root_range(disc, archive, start, data, length, &total)) {
        free(data);
        return NULL;
    }
    *size = length;
    return data;
}

uint8_t *bfm_bedroom_player_data(const MusashiDiscMedia *disc, size_t *size)
{
    return read_payload(disc, "MAIN.CD", 10, 0x101, size);
}

uint8_t *bfm_bedroom_effect_data(const MusashiDiscMedia *disc, unsigned member, size_t *size)
{
    return read_payload(disc, "SC01.CD", member, 0x107, size);
}

int bfm_bedroom_entry(const MusashiDiscMedia *disc, const BfmSaveRecord *record,
                      BfmBedroomEntry *out)
{
    /* MAIN/12 selects its early-story descriptor below story ten. Only
     * the earned story-two wake and its SC01/6 actor graph are supported. */
    enum { EARLY_DESTINATION = 0x58664, EARLY_RESOURCE_ROW = 55 };
    size_t start, length, total;
    uint8_t room[36], group_counts[8], scene_row[8];
    /* Flags 29 through 33 select five additional room setup tables. */
    if ((record->flags[3] & 0xe0u) || (record->flags[4] & 3u)) return 0;
    if (word(record->progress + 8) != 2 ||
        !payload(disc, "MAIN.CD", 12, 0x101, &start, &length) ||
        length < EARLY_DESTINATION + sizeof room ||
        !musashi_disc_media_read_root_range(disc, "MAIN.CD", start + EARLY_DESTINATION,
                                             room, sizeof room, &total) ||
        word(room) != 0x3001 ||
        !musashi_disc_media_read_root_range(disc, "MAIN.CD", 0, group_counts, 4, &total) ||
        !musashi_disc_media_read_root_range(disc, "SC01.CD", 0, group_counts + 4, 4, &total))
        return 0;
    unsigned first = word(group_counts), count = word(group_counts + 4);
    if (first > EARLY_RESOURCE_ROW || EARLY_RESOURCE_ROW - first >= count) return 0;
    unsigned member = EARLY_RESOURCE_ROW - first;
    if (member != 6 || !descriptor(disc, "SC01.CD", member, &start, &length) ||
        !musashi_disc_media_read_root_range(disc, "LIST.CD", 16u + 8u * EARLY_RESOURCE_ROW, scene_row,
                                             sizeof scene_row, &total) ||
        word(scene_row) != start / SECTOR || word(scene_row + 4) != length) return 0;
    out->state = *record;
    memcpy(out->state.destination, room, sizeof room);
    out->scene_member = member;
    return 1;
}

int bfm_castle_room_entry(const MusashiDiscMedia *disc, const uint8_t *transfer, BfmBedroomEntry *entry)
{
    /* The bedroom sends the normal entrance descriptor; the early-story
     * library shares its file numbering with the mounted global directory. */
    if (!disc || !transfer || !entry) return 0;
    uint32_t target = word(transfer);
    if (target != 0x3002u && target != 0x00043000u) return 0;
    unsigned resource = target == 0x3002u ? 57 : 53;
    uint8_t row[8], count[4];
    size_t total, start, length;
    if (!musashi_disc_media_read_root_range(disc, "MAIN.CD", 0, count, sizeof count, &total) ||
        word(count) > resource) return 0;
    unsigned member = resource - word(count);
    if (member != (target == 0x3002u ? 8u : 4u) ||
        !descriptor(disc, "SC01.CD", member, &start, &length) ||
        !musashi_disc_media_read_root_range(disc, "LIST.CD", 16+8*resource,
                                           row, sizeof row, &total) ||
        word(row) != start / SECTOR || word(row+4) != length) return 0;
    memcpy(entry->state.destination, transfer, sizeof(entry->state.destination));
    if (target == 0x3002u) entry->state.progress[8] = 2;
    entry->scene_member = member;
    return 1;
}
