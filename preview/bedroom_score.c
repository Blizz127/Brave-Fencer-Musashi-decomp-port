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

/* These room requests identify particular commands, including two tracks
 * sharing a single archive file. Check the pair before handing the mounted
 * track and gain to the score player. */
static const struct RoomCue {
    unsigned selector, command, archive, member, track;
} twinpeak_cues[] = {
    {0x18, 0x137, 2, 23, 0}, {0x2b, 0x11d, 2, 23, 1},
    {0x26, 0x125, 3, 50, 0}, {0x1c, 0x118, 3, 37, 0},
    {0x1d, 0x119, 3, 38, 0}, {0x1e, 0x11a, 3, 39, 0},
};

/* Resolve the room selector's archive member, track and gain from the
 * mounted US score tables. */
BfmForestScore *bfm_bedroom_score_load(const MusashiDiscMedia *disc, unsigned selector)
{
    uint8_t request[6], command[16], header[4];
    if (selector > 255 || !bfm_bedroom_resident_data(disc, 0x80063138u + 6*selector, request, sizeof request)) return NULL;
    unsigned file = half(request), id = half(request + 4);
    const char *const archives[] = {
        "MAIN.CD", "SC01.CD", "SC02.CD", "SC03.CD", "SC04.CD", "SC05.CD", "SC06.CD"
    };
    const struct RoomCue *cue = NULL;
    for (size_t n = 0; n < sizeof twinpeak_cues / sizeof twinpeak_cues[0]; ++n)
        if (selector == twinpeak_cues[n].selector) cue = &twinpeak_cues[n];
    size_t size;
    for (unsigned i = 0; i < sizeof archives / sizeof *archives; ++i) {
        if (!musashi_disc_media_read_root_range(disc, archives[i], 0, header, sizeof header, &size)) return NULL;
        unsigned count = word(header);
        if (!count || count > size / 8) return NULL;
        if (file < count) {
            int village = i == 3 && file == 0 && (selector == 0x0f || selector == 0x10);
            int station = i == 3 && file == 67 && selector == 0x32;
            if (cue && (i != cue->archive || file != cue->member ||
                        half(request + 2) != cue->command || id != cue->command)) return NULL;
            int bank = i == 2 && file == 34 && ((selector == 0x0a && id == 0x10c) ||
                                                (selector == 0x3c && id == 0x13b));
            int late_bank = i == 6 && file == 38 && selector == 0x79 && id == 0x184;
            int maze = (i == 3 && file == 9 && selector == 0x0b && id == 0x10d) ||
                       (i == 4 && file == 13 && selector == 0x74 && id == 0x150);
            int nightmare = i == 3 && file == 4 && selector == 0x87 &&
                            half(request + 2) == 0x180 && id == 0x180;
            if (((selector == 0x0a || selector == 0x3c) && !bank) ||
                ((selector == 0x0b || selector == 0x74) && !maze) ||
                (selector == 0x79 && !late_bank) ||
                (selector == 0x87 && !nightmare)) return NULL;
            if (!(village || station || nightmare || cue || bank || late_bank || maze ||
                  (i == 1 && file == 75) || (i == 3 && file == 68)) ||
                id < 0x100 || id >= 0x200 ||
                !bfm_bedroom_resident_data(disc,0x80068b5cu+16*(id-0x100),command,sizeof command) ||
                half(command)!=4 || half(command+2)>127) return NULL;
            if (cue) {
                unsigned track = half(command + 6);
                if (track != cue->track) return NULL;
                return bfm_archive_score_load(disc, 37800, archives[i], file, track, half(command + 2));
            }
            if (maze || bank || late_bank) {
                unsigned track = half(command+6);
                if (half(request+2) != id || track != (unsigned)(selector == 0x3c)) return NULL;
                return bfm_archive_score_load(disc, 37800, archives[i], file, track, half(command+2));
            }
            if (nightmare && half(command + 6) != 0) return NULL;
            if (i == 1) return bfm_library_score_load(disc, 37800, half(command+6), half(command+2));
            return bfm_castle_score_load(disc, 37800, file, half(command+6), half(command+2));
        }
        file -= count;
    }
    return NULL;
}
