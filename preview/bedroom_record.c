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
        if (length < SECTOR || length > left) return 0;
        if (word(header + 4) == type) {
            if (length == SECTOR) return 0;
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

uint8_t *bfm_archive_effect_data(const MusashiDiscMedia *disc, const char *archive,
                                 unsigned member, size_t *size)
{
    return read_payload(disc, archive, member, 0x107, size);
}

uint8_t *bfm_bedroom_effect_data(const MusashiDiscMedia *disc, unsigned member, size_t *size)
{
    return bfm_archive_effect_data(disc, "SC01.CD", member, size);
}

/* Village porch events are resident members separate from the field mesh. */
uint8_t *bfm_village_event_data(const MusashiDiscMedia *disc, unsigned member, size_t *size)
{
    return read_payload(disc, "SC03.CD", member, 0x101, size);
}

int bfm_bedroom_entry(const MusashiDiscMedia *disc, const BfmSaveRecord *record,
                      BfmBedroomEntry *out)
{
    /* Continue selects a resident destination when the save has no explicit
     * transfer. The first castle wake and the inn exterior use separate rows. */
    enum { EARLY_DESTINATION = 0x58664, INN_DESTINATION = 0x586f4, EARLY_RESOURCE_ROW = 55 };
    size_t start, length, total;
    uint8_t room[36], group_counts[8], scene_row[8];
    /* Flags 29 through 33 select five additional room setup tables. */
    if (!bfm_save_uses_default_room_table(record)) return 0;
    uint32_t story = word(record->progress + 8);
    if (story != 2 && story != 10) return 0;
    size_t offset = story == 10 ? INN_DESTINATION : EARLY_DESTINATION;
    if (!payload(disc, "MAIN.CD", 12, 0x101, &start, &length) ||
        length < offset + sizeof room ||
        !musashi_disc_media_read_root_range(disc, "MAIN.CD", start + offset,
                                             room, sizeof room, &total)) return 0;
    if (story == 10) {
        if (memcmp(record->destination, (uint8_t[36]){0}, sizeof record->destination) ||
            word(room) != 0x000a1010u) return 0;
        out->state = *record;
        return bfm_castle_room_entry(disc, room, out);
    }
    if (word(room) != 0x3001 ||
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
    if (!disc || !transfer || !entry) return 0;
    uint32_t target = word(transfer);
    unsigned member, resource, group;
    switch (target) {
    case 0x3002: member = 8; resource = 57; group = 1; break;
    case 0x43000: member = 4; resource = 53; group = 1; break;
    case 0x3004: member = 74; resource = 123; group = 1; break;
    case 0x000a1010: case 0x02011010: member = 1; resource = 179; group = 3; break;
    case 0x51011: member = 2; resource = 180; group = 3; break;
    default: return 0;
    }
    /* LIST contains the member rows for each archive in sequence, with a
     * one-row header per archive. Validate its global and local indexes. */
    static const char *const groups[] = {"MAIN.CD", "SC01.CD", "SC02.CD", "SC03.CD"};
    unsigned preceding = 0;
    uint8_t row[8], count[4];
    size_t total, start, length;
    for (unsigned i = 0; i < group; ++i) {
        if (!musashi_disc_media_read_root_range(disc, groups[i], 0, count, sizeof count, &total) ||
            word(count) > resource - preceding) return 0;
        preceding += word(count);
    }
    if (resource - preceding != member ||
        !descriptor(disc, groups[group], member, &start, &length) ||
        !musashi_disc_media_read_root_range(disc, "LIST.CD", 8 * (resource + group + 1),
                                            row, sizeof row, &total) ||
        word(row) != start / SECTOR || word(row + 4) != length) return 0;
    memcpy(entry->state.destination, transfer, sizeof(entry->state.destination));
    if (target == 0x3002u) entry->state.progress[8] = 2;
    /* The field player's room initialization enables world time from story ten. */
    if (word(entry->state.progress + 8) >= 10)
        entry->state.flags[127 / 8] |= 1u << (127 % 8);
    entry->scene_member = member;
    return 1;
}
