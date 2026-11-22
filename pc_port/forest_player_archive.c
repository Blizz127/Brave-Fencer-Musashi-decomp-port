#include "forest_player_archive.h"
#include "forest_player_bytes.h"

#include <string.h>

enum { SECTOR = 2048, PLAYER_MEMBER = 10 };
static const uint32_t player_origin = UINT32_C(0x800cedf8);

int forest_player_archive_open(const MusashiDiscMedia *media, ForestPlayerArchive *out) {
    uint8_t directory[SECTOR], header[16];
    size_t size;
    if (!media || !out ||
        !musashi_disc_media_read_root_range(media, "MAIN.CD", 0, directory,
                                            sizeof(directory), &size)) return 0;
    uint32_t count = u32le(directory);
    if (count <= PLAYER_MEMBER || count > (SECTOR - 8u) / 8u) return 0;
    uint32_t sector = u32le(directory + 8 + PLAYER_MEMBER * 8);
    size_t remaining = u32le(directory + 12 + PLAYER_MEMBER * 8);
    if (!sector || sector > size / SECTOR) return 0;
    size_t cursor = (size_t)sector * SECTOR;
    if (remaining > size - cursor) return 0;
    while (remaining >= SECTOR) {
        if (!musashi_disc_media_read_root_range(media, "MAIN.CD", cursor, header,
                                                sizeof(header), &size) ||
            memcmp(header, "PAC", 4)) return 0;
        size_t length = u32le(header + 12);
        if (length < SECTOR || length > remaining) return 0;
        size_t sectors = (length - 1u) / SECTOR + 1u;
        if (sectors > remaining / SECTOR) return 0;
        if (header[4] == 1) {
            if (header[5] != 1 || length == SECTOR) return 0;
            *out = (ForestPlayerArchive){media, cursor + SECTOR, length - SECTOR};
            return 1;
        }
        cursor += sectors * SECTOR;
        remaining -= sectors * SECTOR;
    }
    return 0;
}

int forest_player_archive_read(const ForestPlayerArchive *archive, uint32_t address,
                               uint8_t *out, size_t bytes) {
    if (!archive || !out || address < player_origin) return 0;
    size_t offset = address - player_origin, total;
    return offset <= archive->size && bytes <= archive->size - offset &&
           musashi_disc_media_read_root_range(archive->media, "MAIN.CD",
               archive->offset + offset, out, bytes, &total);
}
