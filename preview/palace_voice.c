/* SPDX-License-Identifier: MIT */
#include "palace_voice.h"
#include <stdlib.h>
#include <string.h>

enum { GROUPS = 16, SECTOR_FRAMES = 18 * 4 * 28, INTERLEAVE = 8, EXE_HEADER = 2048 };

struct PalaceVoice {
    const MusashiDiscMedia *disc;
    uint32_t archive_start, archive_sectors, group_start[GROUPS];
    size_t cue_directory;
    unsigned half_channels;
    uint32_t next, limit;
    unsigned channel, pending, consumed;
    int last[2], older[2];
    int16_t pcm[SECTOR_FRAMES * 2];
};

static uint32_t little32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static int exe_read(const MusashiDiscMedia *disc, size_t offset, uint8_t *out, size_t count)
{
    size_t size;
    return musashi_disc_media_read_root_range(disc, "SLUS_007.26", offset, out, count, &size);
}

PalaceVoice *bfm_palace_voice_load(const MusashiDiscMedia *disc)
{
    const uint32_t table_address = UINT32_C(0x8006a774);
    uint8_t header[32], directory[8 + 4 * GROUPS];
    uint32_t first, sectors;
    if (!disc || !musashi_disc_media_root_frames(disc, "OPEN.STR", &first, &sectors) ||
        !exe_read(disc, 0, header, sizeof(header)) || memcmp(header, "PS-X EXE", 8)) return NULL;
    uint32_t origin = little32(header + 24);
    if (origin > table_address || !exe_read(disc, EXE_HEADER + (size_t)(table_address - origin),
                                            directory, sizeof(directory))) return NULL;
    uint32_t table = little32(directory);
    if (table < origin || sectors > UINT32_MAX - first) return NULL;
    PalaceVoice *v = calloc(1, sizeof(*v));
    if (!v) return NULL;
    v->disc = disc;
    v->archive_start = first;
    v->archive_sectors = sectors;
    v->cue_directory = EXE_HEADER + (size_t)(table - origin);
    v->half_channels = directory[4] ? 4 : 8;
    for (unsigned i = 0; i < GROUPS; ++i)
        v->group_start[i] = little32(directory + 8 + 4 * i);
    return v;
}

void bfm_palace_voice_stop(PalaceVoice *v)
{
    if (!v) return;
    v->pending = v->consumed = 0;
    v->next = v->limit = 0;
    memset(v->last, 0, sizeof(v->last));
    memset(v->older, 0, sizeof(v->older));
}

void bfm_palace_voice_free(PalaceVoice *v)
{
    free(v);
}

static int audio_sector(const uint8_t *sector, unsigned channel)
{
    return sector[15] == 2 && !memcmp(sector + 16, sector + 20, 4) &&
           sector[16] == 1 && sector[17] == channel &&
           (sector[18] & 0x64) == 0x64 && sector[19] == 1;
}

int bfm_palace_voice_request(PalaceVoice *v, unsigned bank, unsigned cue)
{
    /* OPEN.STR uses stream bank 8. The indexed voice form (bit 14) names
     * sound bank 3 in bits 9..13 and a channel-pair directory below it. */
    if (!v || bank != 8 || (cue & ~0x1ffu) != 0x4600u) return 0;
    unsigned pair = (cue >> 4) & 31u;
    unsigned group = pair / 2;
    unsigned channel = (cue & 15u) + (pair % 2) * v->half_channels;
    if (channel >= INTERLEAVE) return 0;
    uint8_t record[4];
    if (!exe_read(v->disc, v->cue_directory + group * 64 + channel * 4, record, sizeof(record)) ||
        !(record[1] & 0xf0)) return 0;
    uint32_t begin = v->group_start[group];
    uint32_t end = group + 1 < GROUPS ? v->group_start[group + 1] : v->archive_sectors;
    if (end == 0) end = v->archive_sectors;
    if (begin >= end || end > v->archive_sectors || channel >= end - begin) return 0;
    uint8_t sector[MUSASHI_DISC_RAW_SECTOR_SIZE];
    uint32_t first = v->archive_start + begin + channel;
    if (!musashi_disc_media_read_sector(v->disc, first, sector, sizeof(sector)) ||
        !audio_sector(sector, channel)) return 0;
    bfm_palace_voice_stop(v);
    v->next = first;
    v->limit = v->archive_start + end;
    v->channel = channel;
    return 1;
}

static int floor_div64(int x)
{
    return x < 0 ? -((-x + 63) / 64) : x / 64;
}

static int decode(PalaceVoice *v, const uint8_t *sector)
{
    static const int feedback[4][2] = {{0, 0}, {60, 0}, {115, -52}, {98, -55}};
    /* Each group contains four consecutive 28-frame stereo blocks. The
     * low and high nibbles are the left and right channel samples. */
    for (unsigned g = 0; g < 18; ++g) {
        const uint8_t *group = sector + 24 + 128 * g;
        for (unsigned unit = 0; unit < 8; ++unit) {
            unsigned parameter = group[4 + unit];
            unsigned filter = parameter >> 4, shift = parameter & 15u;
            if (filter >= 4 || shift > 12) return 0;
            unsigned ch = unit % 2;
            int a = v->last[ch], b = v->older[ch];
            for (unsigned i = 0; i < 28; ++i) {
                uint8_t byte = group[16 + i * 4 + unit / 2];
                int nibble = (byte >> (4 * ch)) & 15;
                if (nibble >= 8) nibble -= 16;
                int sample = nibble * (1 << (12 - shift));
                sample += floor_div64(a * feedback[filter][0] + b * feedback[filter][1] + 32);
                if (sample > 32767) sample = 32767;
                if (sample < -32768) sample = -32768;
                unsigned frame = (g * 4 + unit / 2) * 28 + i;
                v->pcm[2 * frame + ch] = (int16_t)sample;
                b = a;
                a = sample;
            }
            v->last[ch] = a;
            v->older[ch] = b;
        }
    }
    return 1;
}

size_t bfm_palace_voice_read(PalaceVoice *v, int16_t *stereo, size_t frames)
{
    if (!v || !stereo) return 0;
    size_t done = 0;
    while (done < frames) {
        if (v->consumed == v->pending) {
            uint8_t sector[MUSASHI_DISC_RAW_SECTOR_SIZE];
            if (v->next >= v->limit) break;
            if (!musashi_disc_media_read_sector(v->disc, v->next, sector, sizeof(sector)) ||
                !audio_sector(sector, v->channel) || !decode(v, sector)) {
                bfm_palace_voice_stop(v);
                break;
            }
            if ((sector[18] & 0x80) || v->limit - v->next <= INTERLEAVE) v->next = v->limit;
            else v->next += INTERLEAVE;
            v->pending = SECTOR_FRAMES;
            v->consumed = 0;
        }
        size_t count = v->pending - v->consumed;
        if (count > frames - done) count = frames - done;
        memcpy(stereo + 2 * done, v->pcm + 2 * v->consumed, count * 2 * sizeof(*stereo));
        v->consumed += (unsigned)count;
        done += count;
    }
    return done;
}
