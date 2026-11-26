/* SPDX-License-Identifier: MIT */
#include "bedroom_score.h"
#include <string.h>

static unsigned half(const uint8_t *p)
{
    return p[0] | (unsigned)p[1] << 8;
}

static uint32_t word(const uint8_t *p)
{
    return half(p) | (uint32_t)half(p + 2) << 16;
}

int bfm_bedroom_resident_data(const MusashiDiscMedia *disc, uint32_t address, uint8_t *out, size_t count)
{
    uint8_t header[32];
    size_t size;
    if (!musashi_disc_media_read_root_range(disc, "SLUS_007.26", 0, header, sizeof header, &size) ||
        memcmp(header, "PS-X EXE", 8) || size < 2048) return 0;
    uint32_t base = word(header + 24), bytes = word(header + 28);
    if (address < base || bytes > size - 2048 || count > bytes || address-base > bytes-count) return 0;
    return musashi_disc_media_read_root_range(disc, "SLUS_007.26", 2048u + (address-base), out, count, &size);
}

/* This room uses the resident castle bank. Resolve both its archive member
 * and the selected track from the mounted US score tables. */
BfmForestScore *bfm_bedroom_score_load(const MusashiDiscMedia *disc, unsigned selector)
{
    uint8_t request[6], command[16], header[8];
    if (selector > 255 || !bfm_bedroom_resident_data(disc, 0x80063138u + 6*selector, request, sizeof request)) return NULL;
    unsigned file = half(request), id = half(request + 4);
    const char *const preceding[] = {"MAIN.CD", "SC01.CD", "SC02.CD"};
    size_t size;
    for (unsigned i = 0; i < sizeof preceding / sizeof *preceding; ++i) {
        if (!musashi_disc_media_read_root_range(disc, preceding[i], 0, header, sizeof header, &size)) return NULL;
        unsigned count = word(header);
        if (!count || count > size / 8 || count > file) return NULL;
        file -= count;
    }
    if (file != 68 || id < 0x100 || id >= 0x200 ||
        !bfm_bedroom_resident_data(disc, 0x80068b5cu + 16*(id-0x100), command, sizeof command) ||
        half(command) != 4 || half(command+2) > 127) return NULL;
    return bfm_castle_score_load(disc, 37800, file, half(command+6), half(command+2));
}
