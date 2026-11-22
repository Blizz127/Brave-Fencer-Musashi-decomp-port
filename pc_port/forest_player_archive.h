#ifndef MUSASHI_FOREST_PLAYER_ARCHIVE_H
#define MUSASHI_FOREST_PLAYER_ARCHIVE_H
#include "musashi_disc_media.h"
/* The first-party mounted player archive uses addresses relative to its
 * uncompressed section. Range reads keep each request inside this section. */
typedef struct {
    const MusashiDiscMedia *media;
    size_t offset, size;
} ForestPlayerArchive;
int forest_player_archive_open(const MusashiDiscMedia *media, ForestPlayerArchive *out);
int forest_player_archive_read(const ForestPlayerArchive *archive, uint32_t address,
                               uint8_t *out, size_t bytes);
#endif
