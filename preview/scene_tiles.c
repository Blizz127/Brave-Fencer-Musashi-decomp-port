/* SPDX-License-Identifier: MIT */
#include "scene_tiles.h"
#include <string.h>

enum { SECTOR_BYTES = 2048, TILE_SIDE = 32, TILES_ACROSS = 32, TILE_ROWS = 16 };

static uint32_t read_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static int copy_tile_block(const MusashiDiscMedia *disc, const char *archive,
                           size_t offset, const uint8_t *header, uint32_t bytes,
                           uint16_t words[512][1024])
{
    uint8_t tile[SECTOR_BYTES];
    size_t file_size;
    uint32_t count = read_le32(header + 8);
    uint32_t copied = 0;
    if (!count || count > TILES_ACROSS * TILE_ROWS || bytes != (1 + count) * SECTOR_BYTES)
        return 0;
    for (unsigned ty = 0; ty < TILE_ROWS; ++ty) {
        uint32_t row = read_le32(header + 16 + 4 * ty);
        for (unsigned tx = 0; tx < TILES_ACROSS; ++tx) {
            if (!(row & (UINT32_C(1) << tx)))
                continue;
            if (++copied > count || !musashi_disc_media_read_root_range(disc, archive,
                      offset + copied * SECTOR_BYTES, tile, sizeof(tile), &file_size))
                return 0;
            for (unsigned y = 0; y < TILE_SIDE; ++y) {
                for (unsigned x = 0; x < TILE_SIDE; ++x) {
                    const uint8_t *p = tile + 2 * (TILE_SIDE * y + x);
                    words[TILE_SIDE * ty + y][TILE_SIDE * tx + x] =
                        (uint16_t)(p[0] | (uint16_t)p[1] << 8);
                }
            }
        }
    }
    return copied == count;
}

int bfm_scene_read_tiles(const MusashiDiscMedia *disc, const char *archive,
                         unsigned member, uint16_t words[512][1024])
{
    uint8_t header[SECTOR_BYTES];
    size_t file_size;
    if (!disc || !archive || !words ||
        !musashi_disc_media_read_root_range(disc, archive, 0, header, sizeof(header), &file_size))
        return 0;
    uint32_t members = read_le32(header);
    if (members > (SECTOR_BYTES - 8) / 8 || member >= members)
        return 0;
    uint32_t sector = read_le32(header + 8 + 8 * member);
    size_t remaining = read_le32(header + 12 + 8 * member);
    if (!sector || sector > file_size / SECTOR_BYTES)
        return 0;
    size_t offset = (size_t)sector * SECTOR_BYTES;
    if (remaining > file_size - offset)
        return 0;
    while (remaining >= SECTOR_BYTES) {
        if (!musashi_disc_media_read_root_range(disc, archive, offset, header,
                                                sizeof(header), &file_size) ||
            memcmp(header, "PAC", 4) != 0)
            return 0;
        uint32_t bytes = read_le32(header + 12);
        if (bytes < 16 || bytes > remaining)
            return 0;
        if (header[4] == 0)
            return copy_tile_block(disc, archive, offset, header, bytes, words);
        size_t padded = (((size_t)bytes - 1) / SECTOR_BYTES + 1) * SECTOR_BYTES;
        if (padded > remaining)
            return 0;
        offset += padded;
        remaining -= padded;
    }
    return 0;
}
