/* SPDX-License-Identifier: MIT */
#include "cable_sound.h"
#include "bedroom_record.h"
#include "bedroom_score.h"
#include "palace_sound_bank.h"
#include "spu_envelope.h"
#include "spu_sample.h"

#include <stdlib.h>
#include <string.h>

enum { OUTPUT_RATE = 37800, SPU_RATE = 44100, VOICES = 8, MAX_EVENTS = 256 };
enum { NOTE, LEVEL, RAMP };
enum { STATION_MEMBER = 74, SLIDER_SOUND = 0xbca };

typedef struct {
    unsigned field, kind, pitch, level, gate, step;
} Event;

typedef struct {
    BfmSpuEnvelope envelope;
    size_t phase;
    unsigned pitch, level, gate;
} Voice;

typedef struct {
    Voice voices[VOICES];
    size_t event, sample;
    unsigned field, gain, target, step, release;
    int active, stopping, previous, resample;
} Playback;

struct BfmCableSound {
    BfmDecodedSpuSample sample;
    BfmPalaceTone tone;
    unsigned instrument, bank;
    Event events[MAX_EVENTS];
    size_t count;
    Playback playback;
};

static unsigned half(const uint8_t *p)
{
    return p[0] | (unsigned)p[1] << 8;
}

static uint32_t word(const uint8_t *p)
{
    return half(p) | (uint32_t)half(p + 2) << 16;
}

/* The area directory uses addresses within the mounted sound-data block. */
static const uint8_t *area(const uint8_t *data, size_t bytes, uint32_t address, size_t need)
{
    const uint32_t base = 0x800c7f08u;
    if (address < base || need > bytes || address-base > bytes-need) return NULL;
    return data + (address-base);
}

static int instrument(BfmCableSound *s, const MusashiDiscMedia *disc, BfmPalaceBank *vab,
                      unsigned bank, unsigned number)
{
    if (s->sample.pcm) return s->bank == bank && s->instrument == number;
    uint8_t mapping[12], pair[2];
    BfmSpuSample encoded;
    if (!bfm_bedroom_resident_data(disc, 0x80064d44u + 12*bank, mapping, sizeof mapping) ||
        number >= mapping[4] || word(mapping) > UINT32_MAX - 2*number ||
        !bfm_bedroom_resident_data(disc, word(mapping) + 2*number, pair, sizeof pair) ||
        !bfm_room_effect_tone(vab, pair[0], pair[1], &s->tone) ||
        !bfm_palace_sound_sample(vab, s->tone.sample, &encoded) ||
        !bfm_spu_sample_decode(encoded.data, encoded.bytes, &s->sample)) return 0;
    s->bank = bank;
    s->instrument = number;
    return s->sample.frames == s->sample.loop_start && !s->tone.tuning;
}

static unsigned note_pitch(const uint8_t *table, unsigned key, unsigned center)
{
    int index = (int)key + (60 - (int)center)*256;
    if (index < 0 || index >= 83*256) return 0;
    const uint8_t *p = table + 2*(index/256);
    unsigned fraction = (unsigned)index % 256;
    return (half(p)*(256-fraction) + half(p+2)*fraction) >> 8;
}

/* Expand the finite note repeats at load. Each new note has independent
 * hold and next-event intervals. Short notes inherit the last instrument. */
static int read_sequence(BfmCableSound *s, const MusashiDiscMedia *disc, BfmPalaceBank *vab,
                         const uint8_t *p, const uint8_t *end, unsigned area_bank)
{
    uint8_t volume[256], pitches[168];
    if (!bfm_bedroom_resident_data(disc, 0x8006aa30u, volume, sizeof volume) ||
        !bfm_bedroom_resident_data(disc, 0x8006ab30u, pitches, sizeof pitches)) return 0;
    const uint8_t *repeat = NULL;
    unsigned remaining = 0, field = 0;
    for (unsigned instructions = 0; instructions < 4096 && end-p >= 4; ++instructions) {
        uint32_t control = word(p);
        unsigned op = p[3];
        if (op == 0x82) {
            if (remaining || (control & 0xffff00u) || !p[0]) return 0;
            remaining = p[0];
            repeat = p + 4;
            p = repeat;
            continue;
        }
        if (op == 0x83) {
            if (!remaining || (control & 0xffffffu)) return 0;
            p = --remaining ? repeat : p + 4;
            continue;
        }
        if (s->count >= MAX_EVENTS) return 0;
        Event *event = &s->events[s->count++];
        event->field = field;
        if (op == 0x84 || op == 0x86) {
            event->kind = op == 0x84 ? RAMP : LEVEL;
            event->step = half(p + 1);
            event->level = (p[0] & 127)*16383u >> 7;
            if (p[0] >= 128 || (event->kind == RAMP && !event->step)) return 0;
            p += 4;
            continue;
        }
        size_t length = op == 0x80 ? 20 : 8;
        if ((op != 0x80 && op != 0x81) || (size_t)(end-p) < length || p[2] >= 128) return 0;
        if (op == 0x80 && (p[11] || p[10] != area_bank || word(p+16) ||
                            !instrument(s, disc, vab, p[10], p[9]))) return 0;
        if (!s->sample.pcm) return 0;
        event->kind = NOTE;
        event->gate = half(p+4);
        event->pitch = note_pitch(pitches, half(p), s->tone.center);
        event->level = half(volume + 2*p[2]) * 127u >> 7;
        if (!event->pitch || event->pitch > 16383 || event->gate > 2048) return 0;
        unsigned interval = half(p+6);
        p += length;
        if (!interval) return !remaining && p == end;
        if (interval > 2048-field) return 0;
        field += interval;
    }
    return 0;
}

BfmCableSound *bfm_cable_sound_load(const MusashiDiscMedia *disc)
{
    uint8_t command[4];
    size_t bytes;
    if (!disc || !bfm_bedroom_resident_data(disc, 0x8006451cu + 4*SLIDER_SOUND, command, 4) ||
        command[0] != 5 || command[1] == 0 || command[2]) return NULL;
    uint8_t *data = bfm_bedroom_effect_data(disc, STATION_MEMBER, &bytes);
    if (!data) return NULL;
    BfmPalaceBank *vab = NULL;
    BfmCableSound *s = NULL;
    if (bytes >= 28) {
        const uint8_t *directory = data + bytes - 28;
        unsigned bank = half(directory + 4);
        const uint8_t *rows = area(data, bytes, word(directory + 24), (command[3]+2u)*12);
        if (rows && half(directory) == command[1] && half(directory + 2) == 1 && bank < 128) {
            const uint8_t *row = rows + command[3]*12;
            const uint8_t *start = area(data, bytes, word(row), 4);
            const uint8_t *end = area(data, bytes, word(row+12), 4);
            if (start && end && start < end && end <= rows && row[8] == 0x10 && !half(row+6)) {
                vab = bfm_archive_sound_bank_load(disc, "SC01.CD", STATION_MEMBER);
                s = calloc(1, sizeof *s);
                if (!vab || !s || !read_sequence(s, disc, vab, start, end, bank)) {
                    bfm_cable_sound_free(s);
                    s = NULL;
                }
            }
        }
    }
    bfm_palace_sound_bank_free(vab);
    free(data);
    return s;
}

void bfm_cable_sound_free(BfmCableSound *s)
{
    if (s) {
        bfm_spu_sample_free(&s->sample);
        free(s);
    }
}

void bfm_cable_sound_start(BfmCableSound *s)
{
    if (s) s->playback = (Playback){.active = 1, .gain = 16383, .release = 32767};
}

void bfm_cable_sound_release(BfmCableSound *s)
{
    if (s && s->playback.active) s->playback.stopping = 1;
}

static void field(BfmCableSound *s)
{
    Playback *p = &s->playback;
    if (p->stopping) {
        if (p->release <= 0x220) {
            p->active = 0;
            return;
        }
        p->release -= 0x220;
    }
    if (p->step) {
        unsigned gap = p->gain < p->target ? p->target-p->gain : p->gain-p->target;
        if (gap <= p->step) {
            p->gain = p->target;
            p->step = 0;
        }
        else if (p->gain < p->target) p->gain += p->step;
        else p->gain -= p->step;
    }
    for (unsigned i = 0; i < VOICES; ++i) {
        Voice *v = &p->voices[i];
        if (v->pitch && v->gate && !--v->gate) bfm_spu_envelope_release(&v->envelope);
    }
    while (p->event < s->count && s->events[p->event].field == p->field) {
        const Event *e = &s->events[p->event++];
        if (e->kind != NOTE) {
            p->target = e->level;
            p->step = e->kind == RAMP ? e->step : 0;
            if (!p->step) p->gain = p->target;
        } else {
            for (unsigned i = 0; i < VOICES; ++i) {
                Voice *v = &p->voices[i];
                if (v->pitch) continue;
                *v = (Voice){.pitch = e->pitch, .level = e->level, .gate = e->gate};
                bfm_spu_envelope_start(&v->envelope, s->tone.adsr1, s->tone.adsr2);
                break;
            }
        }
    }
    ++p->field;
}

static int sample(BfmCableSound *s)
{
    Playback *p = &s->playback;
    /* Use the same NTSC field and independent SPU clocks as the room mixer. */
    if (p->sample == (uint64_t)p->field*SPU_RATE*1001/60000) field(s);
    if (!p->active) return 0;
    ++p->sample;
    int result = 0, playing = 0;
    for (unsigned i = 0; i < VOICES; ++i) {
        Voice *v = &p->voices[i];
        if (!v->pitch) continue;
        size_t index = v->phase >> 12;
        int pcm = s->sample.pcm[index];
        if (index+1 < s->sample.frames)
            pcm += (s->sample.pcm[index+1]-pcm)*(int)(v->phase & 4095)/4096;
        unsigned envelope = bfm_spu_envelope_clock(&v->envelope);
        unsigned gain = v->level;
        if (p->stopping) gain = gain*p->release >> 15;
        /* The sequence has three gain controls. Its first one ramps. */
        gain = (gain*p->gain >> 14)*16383 >> 14;
        gain = gain*16383 >> 14;
        result += (int)((int64_t)pcm*envelope*gain/(32767*16384));
        v->phase += v->pitch;
        if ((v->phase >> 12) >= s->sample.frames ||
            (!envelope && v->envelope.stage == BFM_SPU_ENVELOPE_RELEASE)) v->pitch = 0;
        else playing = 1;
    }
    if (!playing && p->event == s->count) p->active = 0;
    return result;
}

void bfm_cable_sound_mix(BfmCableSound *s, int16_t *stereo, size_t frames)
{
    if (!s || !stereo) return;
    Playback *p = &s->playback;
    for (size_t i = 0; i < frames && p->active;) {
        int current = sample(s);
        if (p->resample <= 0) {
            int effect = current + (int)((int64_t)(p->previous-current)*(-p->resample)/OUTPUT_RATE);
            for (unsigned channel = 0; channel < 2; ++channel) {
                int mixed = stereo[2*i+channel] + effect;
                stereo[2*i+channel] = (int16_t)(mixed < -32768 ? -32768 : mixed > 32767 ? 32767 : mixed);
            }
            ++i;
            p->resample += SPU_RATE;
        }
        p->previous = current;
        p->resample -= OUTPUT_RATE;
    }
}
