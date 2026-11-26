/* SPDX-License-Identifier: MIT */
#include "palace_score.h"
#include "palace_sound_bank.h"
#include "spu_envelope.h"
#include "spu_sample.h"
#include "sqv_track.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { VOICES = 24, SAMPLES = 256, CHANNELS = 16, DEVICE_RATE = 44100 };
typedef struct {
    const BfmDecodedSpuSample *sample;
    uint64_t phase;
    unsigned channel, key, pitch, left, right;
    int releasing;
    BfmSpuEnvelope envelope;
} Note;

typedef struct {
    BfmPalaceBank *bank;
    BfmDecodedSpuSample samples[SAMPLES];
    Note notes[VOICES];
    unsigned program[CHANNELS], pan[CHANNELS];
} Orchestra;

static const BfmDecodedSpuSample *instrument(Orchestra *o, unsigned number)
{
    if (!number || number >= SAMPLES) return NULL;
    BfmDecodedSpuSample *s = &o->samples[number];
    if (!s->pcm) {
        BfmSpuSample encoded;
        if (!bfm_palace_sound_sample(o->bank, number, &encoded) ||
            !bfm_spu_sample_decode(encoded.data, encoded.bytes, s)) return NULL;
    }
    return s;
}

/* This response interpolates measured unchanged US port volumes. The central
 * ear keeps its level; the quieter side follows the combined channel/tone pan. */
static unsigned ear_level(unsigned velocity, unsigned pan_weight)
{
    uint64_t q = 4096 * pan_weight / 63;
    int64_t response = 320 + 63 * (int)velocity * ((int)velocity - 6);
    if (response <= 0) return 0;
    const uint64_t denominator = 80ULL * 4096 * 4096;
    return (unsigned)(((uint64_t)response * q * q + denominator - 1) / denominator);
}

static int start_note(Orchestra *o, const BfmSqvEvent *e)
{
    BfmPalaceTone tones[BFM_PALACE_MAX_LAYERS];
    unsigned count = bfm_palace_sound_tones(o->bank, o->program[e->channel], e->number, tones);
    if (!count) return 0;
    for (unsigned t = 0; t < count; ++t) {
        const BfmPalaceTone *tone = &tones[t];
        if (tone->bank_gain != 127 || tone->program_gain != 127 || tone->tone_gain != 127 ||
            tone->bank_pan != 64 || tone->tuning != 0) return 0;
        const BfmDecodedSpuSample *s = instrument(o, tone->sample);
        if (!s) return 0;
        unsigned slot = 0;
        while (slot < VOICES && o->notes[slot].sample) ++slot;
        if (slot == VOICES) return 0;
        Note *n = &o->notes[slot];
        *n = (Note){.sample = s, .channel = e->channel, .key = e->number};
        double pitch = 4096 * exp2(((int)e->number - (int)tone->center) / 12.0);
        if (pitch < 1) return 0;
        /* The top of the mounted phrase saturates at the 14-bit device rate. */
        n->pitch = pitch > 0x3fff ? 0x3fffu : (unsigned)pitch;
        int pan = (int)o->pan[e->channel] + (int)tone->program_pan + (int)tone->tone_pan - 128;
        if (pan < 1 || pan > 127) return 0;
        n->left = ear_level(e->value, pan > 64 ? (unsigned)(127 - pan) : 63);
        n->right = ear_level(e->value, pan < 64 ? (unsigned)(pan - 1) : 63);
        bfm_spu_envelope_start(&n->envelope, (uint16_t)tone->adsr1, (uint16_t)tone->adsr2);
    }
    return 1;
}

static int initial_parameter(const BfmSqvEvent *e)
{
    return e->tick == 0 && e->channel == 0 &&
        (((e->number == 98 || e->number == 99) && e->value == 16) ||
         (e->number == 6 && e->value == 55));
}

/* The paired controller marks the beginning of the repeated court phrase. */
static int first_pass_pair(const BfmSqvTrack *track, size_t i)
{
    if (i + 1 >= track->event_count) return 0;
    const BfmSqvEvent *first = &track->events[i], *second = first + 1;
    return first->kind == BFM_SQV_CONTROL && first->channel == 0 &&
        first->number == 99 && first->value == 20 &&
        second->kind == BFM_SQV_CONTROL && second->channel == first->channel &&
        second->tick == first->tick && second->number == 6 && second->value == 127;
}

static int event(Orchestra *o, const BfmSqvEvent *e)
{
    switch (e->kind) {
    case BFM_SQV_NOTE_ON:
        return start_note(o, e);
    case BFM_SQV_NOTE_OFF:
        for (unsigned i = 0; i < VOICES; ++i) {
            Note *n = &o->notes[i];
            if (n->sample && n->channel == e->channel && n->key == e->number && !n->releasing) {
                n->releasing = 1;
                bfm_spu_envelope_release(&n->envelope);
            }
        }
        return 1;
    case BFM_SQV_PAN:
        o->pan[e->channel] = e->value;
        return 1;
    case BFM_SQV_PROGRAM:
        o->program[e->channel] = e->number;
        return 1;
    case BFM_SQV_CONTROL:
        /* fits_entrance has already checked the controls in this interval. */
        return 1;
    case BFM_SQV_TEMPO:
    case BFM_SQV_END:
        return 1;
    }
    return 0;
}

static void mix_frame(Orchestra *o, int left_right[2])
{
    int64_t left = 0, right = 0;
    for (unsigned i = 0; i < VOICES; ++i) {
        Note *n = &o->notes[i];
        const BfmDecodedSpuSample *s = n->sample;
        if (!s) continue;
        size_t idx = (size_t)(n->phase / 4096), next = idx + 1;
        if (next == s->frames) next = s->loop_start < s->frames ? s->loop_start : idx;
        int v = s->pcm[idx] + (s->pcm[next] - s->pcm[idx]) * (int)(n->phase & 4095) / 4096;
        unsigned envelope = bfm_spu_envelope_clock(&n->envelope);
        left += (int64_t)v * envelope * n->left;
        right += (int64_t)v * envelope * n->right;
        n->phase += n->pitch;
        if (n->releasing && envelope == 0) n->sample = NULL;
        uint64_t end = (uint64_t)s->frames * 4096;
        if (n->phase >= end) {
            if (s->loop_start == s->frames) n->sample = NULL;
            else {
                uint64_t loop = (uint64_t)s->loop_start * 4096;
                n->phase = loop + (n->phase - end) % (end - loop);
            }
        }
    }
    int64_t channels[2] = {left / (16384LL * 32767), right / (16384LL * 32767)};
    for (unsigned c = 0; c < 2; ++c)
        left_right[c] = (int)(channels[c] < INT16_MIN ? INT16_MIN :
                             channels[c] > INT16_MAX ? INT16_MAX : channels[c]);
}

static int render(Orchestra *o, const BfmSqvTrack *track, unsigned rate, size_t frames, int16_t *pcm)
{
    uint32_t tempo = 500000;
    uint64_t denominator = (uint64_t)track->ticks_per_quarter * 1000000;
    uint64_t due = 0, clock = 0;
    size_t index = 0, produced = 0;
    /* Retain fractional tick timing across tempo changes and output samples. */
    uint64_t cursor = 0;
    int previous[2] = {0, 0}, current[2];
    if (!track->event_count || track->events[0].tick != 0) return 0;
    for (uint64_t device = 0; produced < frames; ++device) {
        while (index < track->event_count && due <= clock) {
            const BfmSqvEvent *e = &track->events[index++];
            if (!event(o, e)) return 0;
            if (e->kind == BFM_SQV_TEMPO) tempo = e->tempo_us_per_quarter;
            if (index < track->event_count) {
                uint64_t delta = track->events[index].tick - e->tick;
                uint64_t step = (uint64_t)tempo * DEVICE_RATE;
                if (delta && step > (UINT64_MAX - due) / delta) return 0;
                due += step * delta;
            }
        }
        mix_frame(o, current);
        while (produced < frames && cursor <= device * rate) {
            unsigned fraction = (unsigned)(device * rate - cursor);
            for (unsigned c = 0; c < 2; ++c)
                pcm[2 * produced + c] = (int16_t)(current[c] +
                    ((int64_t)previous[c] - current[c]) * fraction / rate);
            ++produced;
            cursor += DEVICE_RATE;
        }
        previous[0] = current[0];
        previous[1] = current[1];
        clock += denominator;
    }
    return 1;
}

/* Bound the output by the earliest unmodeled parameter change or track end. */
static int fits_entrance(const BfmSqvTrack *track, size_t frames, unsigned rate)
{
    uint32_t tick = 0, tempo = 500000;
    uint64_t elapsed = 0;
    uint64_t divisor = (uint64_t)track->ticks_per_quarter * 1000000;
    for (size_t i = 0; i < track->event_count; ++i) {
        const BfmSqvEvent *e = &track->events[i];
        uint64_t delta = e->tick - tick;
        uint64_t step = (uint64_t)tempo * rate;
        if (delta && step > (UINT64_MAX - elapsed) / delta) return 0;
        elapsed += step * delta;
        tick = e->tick;
        if (first_pass_pair(track, i)) {
            ++i;
            continue;
        }
        if (e->kind == BFM_SQV_END || (e->kind == BFM_SQV_CONTROL && !initial_parameter(e)))
            return frames <= elapsed / divisor;
        if (e->kind == BFM_SQV_TEMPO) tempo = e->tempo_us_per_quarter;
    }
    return 0;
}

/* The first terminal on the mounted court track returns to its marked phrase.
 * Append one traversal while preserving instrument, envelope and channel state.
 * Multiple traversals and the eventual exit are outside the observed scene. */
static int append_court_reprise(BfmSqvTrack *track)
{
    if (track->event_count < 5) return 0;
    size_t terminal = track->event_count - 2;
    const BfmSqvEvent *close = &track->events[terminal];
    if (close->kind != BFM_SQV_CONTROL || close->channel != 0 ||
        close->number != 99 || close->value != 30 ||
        track->events[terminal + 1].kind != BFM_SQV_END ||
        track->events[terminal + 1].tick != close->tick) return 0;

    size_t start = 0;
    for (size_t i = 0; i < terminal; ++i) {
        if (first_pass_pair(track, i)) {
            if (start) return 0;
            start = i + 2;
        }
    }
    if (!start || start >= terminal) return 0;
    uint32_t mark = track->events[start - 1].tick;
    if (mark >= close->tick) return 0;
    uint32_t length = close->tick - mark;
    if (length > UINT32_MAX - close->tick) return 0;
    size_t added = terminal - start;
    if (added > SIZE_MAX / sizeof(*track->events) - track->event_count) return 0;
    size_t count = track->event_count + added;
    BfmSqvEvent *events = realloc(track->events, count * sizeof(*events));
    if (!events) return 0;
    memmove(events + terminal, events + start, (count - terminal) * sizeof(*events));
    for (size_t i = terminal; i < count; ++i) events[i].tick += length;
    track->duration_ticks = events[count - 1].tick;
    track->events = events;
    track->event_count = count;
    return 1;
}

int bfm_palace_score_render(const MusashiDiscMedia *disc, unsigned rate, size_t frames, int16_t **stereo)
{
    if (!stereo) return 0;
    *stereo = NULL;
    if (!disc || !frames || frames > SIZE_MAX / (2 * sizeof(int16_t)) ||
        (rate != DEVICE_RATE && rate != 37800)) return 0;
    BfmSqvTrack track;
    if (bfm_sqv_track_load(disc, 1, &track) != BFM_SQV_OK) return 0;
    /* The observed reprise reaches the close of the reply 3686 original fields
     * after the second flash. Do not infer another traversal from the marker. */
    int supported = fits_entrance(&track, frames, rate);
    if (!supported && frames <= UINT64_C(3686) * rate * 1001 / 60000 &&
        append_court_reprise(&track))
        supported = fits_entrance(&track, frames, rate);
    int16_t *out = supported ? malloc(2 * frames * sizeof(*out)) : NULL;
    Orchestra *o = out ? calloc(1, sizeof(*o)) : NULL;
    int ok = 0;
    if (o) {
        o->bank = bfm_palace_sound_bank_load(disc, 2);
        for (unsigned i = 0; i < CHANNELS; ++i) o->pan[i] = 64;
        if (o->bank) ok = render(o, &track, rate, frames, out);
        bfm_palace_sound_bank_free(o->bank);
        for (unsigned i = 0; i < SAMPLES; ++i) bfm_spu_sample_free(&o->samples[i]);
        free(o);
    }
    bfm_sqv_track_free(&track);
    if (ok) *stereo = out;
    else free(out);
    return ok;
}
