/* SPDX-License-Identifier: MIT */
#include "disc_voice.h"
#include <stdlib.h>
#include <string.h>

enum { GROUPS = 16, STEREO_FRAMES = 18 * 4 * 28, MONO_FRAMES = 2 * STEREO_FRAMES, EXE_HEADER = 2048 };

typedef struct {
    const char *file;
    uint32_t table_address;
    unsigned script_bank, prefix;
} VoiceArchive;

static const VoiceArchive archives[] = {
    [BFM_VOICE_OPEN] = { "OPEN.STR", 0x8006a774, 8, 0x4600 },
    [BFM_VOICE_ST01] = { "ST01.STR", 0x8006a69c, 0, 0x4000 },
    [BFM_VOICE_ST02] = { "ST02.STR", 0x8006a6e4, 1, 0x4200 },
    [BFM_VOICE_SHOPS01] = { "SHOPS01.STR", 0x8006a72c, 2, 0x4400 }
};

struct PalaceVoice {
    const MusashiDiscMedia *disc;
    uint32_t archive_start, archive_sectors, group_start[GROUPS];
    size_t cue_directory;
    const VoiceArchive *archive;
    unsigned half_channels, interleave;
    uint32_t next, limit;
    unsigned channel, pending, consumed;
    int last[2], older[2];
    int16_t pcm[MONO_FRAMES * 2];
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

DiscVoice *bfm_disc_voice_load(const MusashiDiscMedia *disc, BfmDiscVoiceArchive archive)
{
    if ((unsigned)archive >= sizeof(archives) / sizeof(archives[0])) return NULL;
    const VoiceArchive *spec = &archives[archive];
    const uint32_t table_address = spec->table_address;
    uint8_t header[32], directory[8 + 4 * GROUPS];
    uint32_t first, sectors;
    if (!disc || !musashi_disc_media_root_frames(disc, spec->file, &first, &sectors) ||
        !exe_read(disc, 0, header, sizeof(header)) || memcmp(header, "PS-X EXE", 8)) return NULL;
    uint32_t origin = little32(header + 24);
    if (origin > table_address || !exe_read(disc, EXE_HEADER + (size_t)(table_address - origin),
                                            directory, sizeof(directory))) return NULL;
    uint32_t table = little32(directory);
    if (table < origin || sectors > UINT32_MAX - first) return NULL;
    DiscVoice *v = calloc(1, sizeof(*v));
    if (!v) return NULL;
    v->disc = disc;
    v->archive = spec;
    v->archive_start = first;
    v->archive_sectors = sectors;
    v->cue_directory = EXE_HEADER + (size_t)(table - origin);
    v->half_channels = directory[4] ? 4 : 8;
    v->interleave = v->half_channels * 2;
    for (unsigned i = 0; i < GROUPS; ++i)
        v->group_start[i] = little32(directory + 8 + 4 * i);
    return v;
}

void bfm_disc_voice_stop(DiscVoice *v)
{
    if (!v) return;
    v->pending = v->consumed = 0;
    v->next = v->limit = 0;
    memset(v->last, 0, sizeof(v->last));
    memset(v->older, 0, sizeof(v->older));
}

void bfm_disc_voice_free(DiscVoice *v)
{
    free(v);
}

static int audio_sector(const uint8_t *sector, unsigned channel, unsigned interleave)
{
    return sector[15] == 2 && !memcmp(sector + 16, sector + 20, 4) &&
           sector[16] == 1 && sector[17] == channel &&
           (sector[18] & 0x64) == 0x64 && sector[19] == (interleave == 8 ? 1 : 0);
}

int bfm_disc_voice_request(DiscVoice *v, unsigned bank, unsigned cue, BfmDiscVoiceCue *selected)
{
    if (!v) return 0;
    /* The mounted descriptor maps each pair of selector rows onto one
     * interleaved group. Bits 9..13 select the sound archive. */
    if (bank != v->archive->script_bank || (cue & ~0x1ffu) != v->archive->prefix) return 0;
    unsigned pair = (cue >> 4) & 31u;
    unsigned group = pair / 2;
    unsigned channel = (cue & 15u) + (pair % 2) * v->half_channels;
    if (channel >= v->interleave) return 0;
    uint8_t record[4];
    if (!exe_read(v->disc, v->cue_directory + group * 64 + channel * 4, record, sizeof(record)) ||
        !(record[1] & 0xf0)) return 0;
    uint32_t begin = v->group_start[group];
    uint32_t end = group + 1 < GROUPS ? v->group_start[group + 1] : v->archive_sectors;
    if (end == 0) end = v->archive_sectors;
    if ((group && !begin) || begin >= end || end > v->archive_sectors || channel >= end - begin) return 0;
    uint8_t sector[MUSASHI_DISC_RAW_SECTOR_SIZE];
    uint32_t first = v->archive_start + begin + channel;
    if (!musashi_disc_media_read_sector(v->disc, first, sector, sizeof(sector)) ||
        !audio_sector(sector, channel, v->interleave)) return 0;
    bfm_disc_voice_stop(v);
    v->next = first;
    v->limit = v->archive_start + end;
    v->channel = channel;
    if (selected) {
        selected->channel_offset = channel;
        selected->group_head_frames = 0;
        if (channel == 0)
            selected->group_head_frames = v->interleave == 8 ? STEREO_FRAMES : MONO_FRAMES;
        selected->sound_ticks = (unsigned)record[2] | (unsigned)record[3] << 8;
    }
    return 1;
}

static int floor_div64(int x)
{
    return x < 0 ? -((-x + 63) / 64) : x / 64;
}

static int decode(DiscVoice *v, const uint8_t *sector)
{
    static const int feedback[4][2] = {{0, 0}, {60, 0}, {115, -52}, {98, -55}};
    /* Stereo alternates channels between the low and high nibble units.
     * In a mono sector all eight units advance one shared predictor. */
    unsigned channels = v->interleave == 8 ? 2 : 1;
    for (unsigned g = 0; g < 18; ++g) {
        const uint8_t *group = sector + 24 + 128 * g;
        for (unsigned unit = 0; unit < 8; ++unit) {
            unsigned parameter = group[4 + unit];
            unsigned filter = parameter >> 4, shift = parameter & 15u;
            if (filter >= 4 || shift > 12) return 0;
            unsigned ch = unit % channels;
            int a = v->last[ch], b = v->older[ch];
            for (unsigned i = 0; i < 28; ++i) {
                uint8_t byte = group[16 + i * 4 + unit / 2];
                int nibble = (byte >> (4 * (unit % 2))) & 15;
                if (nibble >= 8) nibble -= 16;
                int sample = nibble * (1 << (12 - shift));
                sample += floor_div64(a * feedback[filter][0] + b * feedback[filter][1] + 32);
                if (sample > 32767) sample = 32767;
                if (sample < -32768) sample = -32768;
                unsigned frame = ((g * 8 + unit) / channels) * 28 + i;
                v->pcm[2 * frame + ch] = (int16_t)sample;
                if (channels == 1) v->pcm[2 * frame + 1] = (int16_t)sample;
                b = a;
                a = sample;
            }
            v->last[ch] = a;
            v->older[ch] = b;
        }
    }
    return 1;
}

int bfm_disc_voice_prime(DiscVoice *v)
{
    if (!v) return 0;
    if (v->consumed < v->pending) return 1;
    if (v->next >= v->limit) return 0;
    uint8_t sector[MUSASHI_DISC_RAW_SECTOR_SIZE];
    if (!musashi_disc_media_read_sector(v->disc, v->next, sector, sizeof(sector)) ||
        !audio_sector(sector, v->channel, v->interleave) || !decode(v, sector)) {
        bfm_disc_voice_stop(v);
        return 0;
    }
    if ((sector[18] & 0x80) || v->limit - v->next <= v->interleave) v->next = v->limit;
    else v->next += v->interleave;
    v->pending = v->interleave == 8 ? STEREO_FRAMES : MONO_FRAMES;
    v->consumed = 0;
    return 1;
}

size_t bfm_disc_voice_read(DiscVoice *v, int16_t *stereo, size_t frames)
{
    if (!v || !stereo) return 0;
    size_t done = 0;
    while (done < frames && bfm_disc_voice_prime(v)) {
        size_t count = v->pending - v->consumed;
        if (count > frames - done) count = frames - done;
        memcpy(stereo + 2 * done, v->pcm + 2 * v->consumed, count * 2 * sizeof(*stereo));
        v->consumed += (unsigned)count;
        done += count;
    }
    return done;
}
