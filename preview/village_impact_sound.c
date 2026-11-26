/* SPDX-License-Identifier: MIT */
#include "village_impact_sound.h"

#include "bedroom_score.h"
#include "palace_sound_bank.h"
#include "spu_envelope.h"
#include "spu_sample.h"

#include <stdlib.h>
#include <string.h>

enum { MEMBER = 2, COMMAND = 0x842, LAYERS = 3, SECTOR = 2048 };
enum { SPU_RATE = 44100, OUTPUT_RATE = 37800, PITCH_UNIT = 4096 };

struct BfmVillageImpactSound {
    int16_t *mono;
    size_t frames, position;
};

typedef struct {
    BfmSpuEnvelope envelope;
    uint64_t phase;
    unsigned pitch, gain;
} Voice;

static unsigned half(const uint8_t *p)
{
    return p[0] | (unsigned)p[1] << 8;
}

static uint32_t word(const uint8_t *p)
{
    return half(p) | (uint32_t)half(p+2) << 16;
}

/* The uncompressed area sound directory follows the VAB in the room member. */
static uint8_t *read_area(const MusashiDiscMedia *disc, size_t *bytes)
{
    uint8_t header[16], directory[8+8*(MEMBER+1)];
    size_t size;
    if (!musashi_disc_media_read_root_range(disc, "SC03.CD", 0, directory, sizeof directory, &size) ||
        word(directory) <= MEMBER) return NULL;
    const uint8_t *member = directory+8+8*MEMBER;
    uint32_t sector = word(member);
    if (!sector || sector > size/SECTOR) return NULL;
    size_t start = (size_t)sector*SECTOR, left = word(member+4);
    if (left > size-start) return NULL;
    while (left >= SECTOR) {
        if (!musashi_disc_media_read_root_range(disc, "SC03.CD", start, header, sizeof header, &size) ||
            memcmp(header, "PAC", 4)) return NULL;
        uint32_t length = word(header+12);
        if (length <= SECTOR || length > left) return NULL;
        if (word(header+4) == 0x107) {
            size_t count = length-SECTOR;
            if (count < 28 || count > 65536) return NULL;
            uint8_t *data = malloc(count);
            if (!data || !musashi_disc_media_read_root_range(disc, "SC03.CD", start+SECTOR, data, count, &size)) {
                free(data);
                return NULL;
            }
            *bytes = count;
            return data;
        }
        size_t sectors = 1+(length-1)/SECTOR;
        if (sectors > left/SECTOR) return NULL;
        start += sectors*SECTOR;
        left -= sectors*SECTOR;
    }
    return NULL;
}

static const uint8_t *area_at(const uint8_t *data, size_t bytes, uint64_t address, size_t count)
{
    const uint32_t base = 0x800c7f08u;
    if (address < base || count > bytes || address-base > bytes-count) return NULL;
    return data + (size_t)(address-base);
}

static const uint8_t *landing_layers(const MusashiDiscMedia *disc, const uint8_t *data, size_t bytes,
                                     unsigned *bank)
{
    uint8_t command[4];
    const uint8_t *directory = data+bytes-28;
    if (!bfm_bedroom_resident_data(disc, 0x8006451cu+4*COMMAND, command, sizeof command) ||
        command[0] != 1 || command[1] != half(directory) || half(directory+2) != 1) return NULL;
    *bank = half(directory+4);
    const uint8_t *table = area_at(data, bytes, (uint64_t)word(directory+20)+4*command[2], 4);
    return table ? area_at(data, bytes, (uint64_t)word(table)+20*command[3], 20*LAYERS) : NULL;
}

static int open_voice(const MusashiDiscMedia *disc, const BfmPalaceBank *vab, const uint8_t *description,
                      unsigned bank, Voice *voice, unsigned *sample)
{
    uint8_t map[12], pair[2], volume[2], pitch[2];
    unsigned instrument = description[9];
    /* These simultaneous centered notes have zero initial delay and no pitch
     * or volume control stream. Each tone supplies its own envelope and center. */
    if (bank != description[8] || half(description+2) || description[4] ||
        description[7] || description[6] >= 128 || word(description+12) || word(description+16) ||
        !bfm_bedroom_resident_data(disc, 0x80064d44u+12*bank, map, sizeof map) ||
        instrument >= map[4] || word(map) > UINT32_MAX-2*instrument ||
        !bfm_bedroom_resident_data(disc, word(map)+2*instrument, pair, sizeof pair)) return 0;
    BfmPalaceTone tone;
    if (!bfm_room_effect_tone(vab, pair[0], pair[1], &tone) || tone.tuning ||
        (*sample && *sample != tone.sample)) return 0;
    *sample = tone.sample;
    int key = description[5]+60-(int)tone.center;
    if (key < 0 || key >= 84 ||
        !bfm_bedroom_resident_data(disc, 0x8006ab30u+2*(unsigned)key, pitch, sizeof pitch) ||
        !bfm_bedroom_resident_data(disc, 0x8006aa30u+2*description[6], volume, sizeof volume)) return 0;
    voice->pitch = half(pitch);
    voice->gain = ((half(volume)*127 >> 7)*127) >> 7;
    bfm_spu_envelope_start(&voice->envelope, (uint16_t)tone.adsr1, (uint16_t)tone.adsr2);
    return voice->pitch && voice->pitch <= 16383 && voice->gain <= 16383;
}

static int voice_sample(Voice *v, const BfmDecodedSpuSample *sample)
{
    size_t index = (size_t)(v->phase/PITCH_UNIT);
    if (index >= sample->frames) return 0;
    int value = sample->pcm[index];
    if (index+1 < sample->frames)
        value += (sample->pcm[index+1]-value)*(int)(v->phase%PITCH_UNIT)/PITCH_UNIT;
    v->phase += v->pitch;
    return (int)((int64_t)value*bfm_spu_envelope_clock(&v->envelope)*v->gain/(32767*16384));
}

static int16_t limit(int value)
{
    return (int16_t)(value < INT16_MIN ? INT16_MIN : value > INT16_MAX ? INT16_MAX : value);
}

static BfmVillageImpactSound *render(Voice voices[LAYERS], const BfmDecodedSpuSample *sample)
{
    unsigned slowest = voices[0].pitch;
    for (unsigned i=1; i<LAYERS; ++i)
        if (voices[i].pitch < slowest) slowest = voices[i].pitch;
    uint64_t ticks = ((uint64_t)sample->frames*PITCH_UNIT+slowest-1)/slowest;
    if (!ticks || ticks > SPU_RATE*8) return NULL;
    size_t frames = (size_t)(1+(ticks-1)*OUTPUT_RATE/SPU_RATE);
    BfmVillageImpactSound *sound = calloc(1, sizeof *sound);
    if (!sound) return NULL;
    sound->mono = malloc(frames*sizeof *sound->mono);
    if (!sound->mono) {
        free(sound);
        return NULL;
    }
    int previous = 0, phase = 0;
    for (uint64_t t=0; t<ticks; ++t) {
        int current = 0;
        for (unsigned i=0; i<LAYERS; ++i) current += voice_sample(&voices[i], sample);
        if (phase <= 0) {
            sound->mono[sound->frames++] = limit(current+(int)((int64_t)(previous-current)*(-phase)/OUTPUT_RATE));
            phase += SPU_RATE;
        }
        previous = current;
        phase -= OUTPUT_RATE;
    }
    sound->position = sound->frames;
    return sound;
}

BfmVillageImpactSound *bfm_village_impact_sound_load(const MusashiDiscMedia *disc)
{
    size_t bytes;
    if (!disc) return NULL;
    uint8_t *data = read_area(disc, &bytes);
    if (!data) return NULL;
    unsigned bank, sample = 0;
    const uint8_t *layers = landing_layers(disc, data, bytes, &bank);
    BfmPalaceBank *vab = bfm_archive_sound_bank_load(disc, "SC03.CD", MEMBER);
    Voice voices[LAYERS] = {0};
    int ready = vab && layers;
    for (unsigned i=0; ready && i<LAYERS; ++i) {
        const uint8_t *d = layers+20*i;
        ready = !!d[11] == (i+1 < LAYERS) && open_voice(disc, vab, d, bank, &voices[i], &sample);
    }
    BfmSpuSample encoded;
    BfmDecodedSpuSample decoded = {0};
    BfmVillageImpactSound *sound = NULL;
    if (ready && bfm_palace_sound_sample(vab, sample, &encoded) &&
        bfm_spu_sample_decode(encoded.data, encoded.bytes, &decoded) && decoded.loop_start == decoded.frames)
        sound = render(voices, &decoded);
    bfm_spu_sample_free(&decoded);
    bfm_palace_sound_bank_free(vab);
    free(data);
    return sound;
}

void bfm_village_impact_sound_free(BfmVillageImpactSound *sound)
{
    if (sound) free(sound->mono);
    free(sound);
}

void bfm_village_impact_sound_start(BfmVillageImpactSound *sound)
{
    if (sound) sound->position = 0;
}

void bfm_village_impact_sound_mix(BfmVillageImpactSound *sound, int16_t *stereo, size_t frames)
{
    if (!sound || !stereo) return;
    size_t left = sound->frames-sound->position;
    if (frames > left) frames = left;
    for (size_t i=0; i<frames; ++i) {
        int value = sound->mono[sound->position++];
        stereo[2*i] = limit(stereo[2*i]+value);
        stereo[2*i+1] = limit(stereo[2*i+1]+value);
    }
}
