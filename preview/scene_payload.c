/* SPDX-License-Identifier: MIT */
#include "scene_payload.h"

#include <stdlib.h>
#include <string.h>

enum { SECTOR_BYTES = 2048, DICTIONARY = 1024 };

static uint32_t read32(const uint8_t *p)
{
    return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static size_t unpack(const uint8_t *src, size_t length, uint8_t *dst, size_t capacity)
{
    uint8_t ring[DICTIONARY] = {0}, filled[DICTIONARY] = {0};
    unsigned cursor = 1;
    size_t i = 0, n = 0;

    while (i < length) {
        unsigned flags = src[i++];
        for (unsigned bit = 0; bit < 8; ++bit, flags >>= 1) {
            if (flags & 1) {
                if (i == length || n == capacity)
                    return 0;
                dst[n++] = ring[cursor] = src[i++];
                filled[cursor] = 1;
                cursor = (cursor + 1) & (DICTIONARY - 1);
            } else {
                if (length - i < 2)
                    return 0;
                unsigned token = src[i] | (unsigned)src[i + 1] << 8;
                unsigned from = token & (DICTIONARY - 1);
                unsigned count = (token >> 10) + 2;
                i += 2;
                if (!from)
                    return n;
                if (count > capacity - n)
                    return 0;
                while (count--) {
                    if (!filled[from])
                        return 0;
                    dst[n++] = ring[cursor] = ring[from];
                    filled[cursor] = 1;
                    cursor = (cursor + 1) & (DICTIONARY - 1);
                    from = (from + 1) & (DICTIONARY - 1);
                }
            }
        }
    }
    return 0;
}

uint8_t *bfm_scene_unpack_pac0(const MusashiDiscMedia *disc, const char *archive,
                               unsigned member, size_t packed_limit,
                               size_t output_limit, size_t *size)
{
    uint8_t header[SECTOR_BYTES];
    size_t archive_size;
    if (!size)
        return NULL;
    *size = 0;
    if (!disc || !archive || !packed_limit || !output_limit ||
        !musashi_disc_media_read_root_range(disc, archive, 0, header, sizeof(header),
                                             &archive_size))
        return NULL;
    unsigned count = read32(header);
    if (count > (SECTOR_BYTES - 8) / 8 || member >= count)
        return NULL;
    size_t sector = read32(header + 8 + member * 8);
    size_t extent = read32(header + 12 + member * 8);
    if (!sector || sector > archive_size / SECTOR_BYTES)
        return NULL;
    size_t offset = sector * SECTOR_BYTES;
    if (extent < SECTOR_BYTES || extent > archive_size - offset ||
        !musashi_disc_media_read_root_range(disc, archive, offset, header, 16, &archive_size) ||
        memcmp(header, "PAC", 4) || read32(header + 4) != 4 || read32(header + 8) != 0)
        return NULL;
    size_t span = read32(header + 12);
    if (span <= SECTOR_BYTES || span > extent)
        return NULL;
    size_t packed_size = span - SECTOR_BYTES;
    if (packed_size > packed_limit)
        return NULL;
    uint8_t *packed = malloc(packed_size);
    uint8_t *payload = malloc(output_limit);
    if (packed && payload &&
        musashi_disc_media_read_root_range(disc, archive, offset + SECTOR_BYTES,
                                            packed, packed_size, &archive_size))
        *size = unpack(packed, packed_size, payload, output_limit);
    free(packed);
    if (!*size) {
        free(payload);
        return NULL;
    }
    return payload;
}
