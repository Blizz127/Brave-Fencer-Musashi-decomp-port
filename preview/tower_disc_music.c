/* SPDX-License-Identifier: MIT */
#include "tower_disc_music.h"
#include "bedroom_score.h"

#include <stdlib.h>

enum { CD_FRAMES = 588, OUTPUT_FRAMES = 504 };

struct BfmTowerDiscMusic {
    size_t cursor, length;
    unsigned dry_level, gain, fade_phase, service_clock;
    int16_t pcm[];
};

static uint16_t little16(const uint8_t *p)
{
    return (uint16_t)p[0] | (uint16_t)((unsigned)p[1] << 8);
}

BfmTowerDiscMusic *bfm_tower_disc_music_load(const MusashiDiscMedia *disc, unsigned entrance)
{
    if (!disc || (entrance != 2 && entrance != 3)) return NULL;
    /* Each mounted room callback selects a resident audio-track request.
     * A negative file field routes the request to the optical player. */
    uint8_t row[6], volume;
    if (!bfm_bedroom_resident_data(disc, UINT32_C(0x80063138) + 6 * (entrance + 4),
                                   row, sizeof row) || little16(row) != UINT16_MAX ||
        little16(row + 4) || !bfm_bedroom_resident_data(disc, UINT32_C(0x8006a994) + 7,
                                                        &volume, sizeof volume) || volume > 127)
        return NULL;
    MusashiDiscMediaInfo info;
    unsigned track = little16(row + 2);
    if (track != entrance || !musashi_disc_media_get_info(disc, &info) ||
        info.sector_size != MUSASHI_DISC_RAW_SECTOR_SIZE || track > info.track_count) return NULL;
    const MusashiDiscTrackInfo *extent = &info.tracks[track - 1];
    if (extent->number != track || extent->mode != MUSASHI_DISC_TRACK_AUDIO ||
        extent->end_frame > info.total_frames || extent->index01_frame >= extent->end_frame)
        return NULL;
    /* GetTD returns minutes and seconds. The optical player seeks that
     * location at frame zero, within the track's pregap. */
    uint32_t start = extent->index01_frame - extent->index01_frame % 75;
    if (start < extent->index00_frame) return NULL;
    size_t sectors = extent->end_frame - start;
    if (sectors > (SIZE_MAX - sizeof(BfmTowerDiscMusic)) / MUSASHI_DISC_RAW_SECTOR_SIZE) return NULL;
    BfmTowerDiscMusic *music = malloc(sizeof(*music) + sectors * MUSASHI_DISC_RAW_SECTOR_SIZE);
    if (!music) return NULL;
    *music = (BfmTowerDiscMusic){ .length = sectors * OUTPUT_FRAMES, .dry_level = 97u * volume / 128u };
    music->gain = 258 * music->dry_level;
    uint8_t raw[MUSASHI_DISC_RAW_SECTOR_SIZE];
    for (size_t i = 0; i < sectors; ++i) {
        if (!musashi_disc_media_read_sector(disc, start + (uint32_t)i, raw, sizeof raw)) {
            free(music);
            return NULL;
        }
        for (unsigned s = 0; s < 2 * CD_FRAMES; ++s) {
            int sample = little16(raw + 2 * s);
            music->pcm[i * 2 * CD_FRAMES + s] = (int16_t)(sample >= 32768 ? sample - 65536 : sample);
        }
    }
    return music;
}

void bfm_tower_disc_music_fade(BfmTowerDiscMusic *music)
{
    if (music && !music->fade_phase) music->fade_phase = UINT16_MAX;
}

static int dry_sample(int16_t sample, unsigned gain)
{
    int level = sample * (int)gain;
    return level < 0 ? -((-level + 32767) / 32768) : level / 32768;
}

size_t bfm_tower_disc_music_mix(BfmTowerDiscMusic *music, int16_t *out, size_t frames)
{
    if (!music || !out) return 0;
    size_t written = 0;
    while (written < frames && music->cursor < music->length) {
        /* One CD sector contains 588 stereo frames and spans exactly 504
         * output frames. Interpolate on that shared boundary without drift. */
        size_t sixth = music->cursor % 6;
        size_t source = music->cursor / 6 * 7 + sixth;
        for (unsigned ch = 0; ch < 2; ++ch) {
            int a = dry_sample(music->pcm[2 * source + ch], music->gain);
            int b = dry_sample(music->pcm[2 * (source + 1) + ch], music->gain);
            int value = a + (b - a) * (int)sixth / 6 + out[2 * written + ch];
            if (value > INT16_MAX) value = INT16_MAX;
            if (value < INT16_MIN) value = INT16_MIN;
            out[2 * written + ch] = (int16_t)value;
        }
        ++music->cursor;
        ++written;
        if (music->fade_phase) {
            /* The ordinary field loop services optical fades once per two NTSC fields. */
            music->service_clock += 30000;
            if (music->service_clock >= 37800 * 1001) {
                music->service_clock -= 37800 * 1001;
                music->gain = 258u * ((music->fade_phase * music->dry_level) >> 16);
                if (music->fade_phase <= 1023u) music->cursor = music->length;
                else music->fade_phase -= 1023u;
            }
        }
    }
    return written;
}

void bfm_tower_disc_music_free(BfmTowerDiscMusic *music)
{
    free(music);
}
