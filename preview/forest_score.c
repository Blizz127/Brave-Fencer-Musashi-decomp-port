/* SPDX-License-Identifier: MIT */
#include "forest_score.h"
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
    unsigned channel, key, center, pitch, left, right, bend_range[2];
    int releasing;
    BfmSpuEnvelope envelope;
} Note;

typedef struct {
    BfmPalaceBank *bank;
    BfmDecodedSpuSample samples[SAMPLES];
    Note notes[VOICES];
    unsigned program[CHANNELS], pan[CHANNELS], wheel[CHANNELS];
    int samples_ready;
} Orchestra;

struct BfmForestScore {
    Orchestra orchestra;
    BfmSqvTrack track;
    uint64_t *deadlines, clock, divisor, repeat_period;
    uint64_t field_duration, first_field_phase, field_phase, bend_deadline;
    const BfmSqvEvent *pending_bend;
    size_t count, event, frames, remaining, phrase;
    unsigned rate;
    int previous[2], until_output, failed;
};

static int16_t limit_sample(int64_t n)
{
    return (int16_t)(n < INT16_MIN ? INT16_MIN : n > INT16_MAX ? INT16_MAX : n);
}

static unsigned note_pitch(unsigned key, unsigned center, unsigned wheel, unsigned range)
{
    /* The upward and downward pitch steps were measured at the original device. */
    double bend = wheel < 64 ? ((int)wheel - 64) * (int)range / 64.0 : 0;
    if (wheel == 67) bend = 17.0 / 128.0;
    double interval = (int)key - (int)center + bend;
    double pitch = 4096.0 * exp2(interval / 12.0);
    return pitch > 16383 ? 16383 : (unsigned)pitch;
}

static const BfmDecodedSpuSample *instrument(Orchestra *o, unsigned number)
{
    if (!number || number >= SAMPLES) return NULL;
    BfmDecodedSpuSample *s = &o->samples[number];
    if (!s->pcm) {
        if (o->samples_ready) return NULL;
        BfmSpuSample encoded;
        if (!bfm_palace_sound_sample(o->bank, number, &encoded) ||
            !bfm_spu_sample_decode(encoded.data, encoded.bytes, s)) return NULL;
    }
    return s;
}

/* The strong ear follows the measured velocity response. At the top end
 * interpolate between the independently observed 120 and 125 levels. For
 * the quieter ear the registered gain follows the squared pan weight. */
static unsigned ear_level(unsigned velocity, unsigned pan_weight)
{
    double position = velocity > 3 ? velocity - 3 : 0;
    if (velocity > 120) position += (velocity < 125 ? velocity - 120 : 5) / 5.0;
    double level = 0.71413 * position * position - 1.48;
    if (level <= 0) return 0;
    unsigned raw = (unsigned)(level * pan_weight * pan_weight / (63.0 * 63.0) + 0.5);
    return raw * 127 / 128;
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
        if (o->wheel[e->channel] != 64) return 0;
        n->center = tone->center;
        n->bend_range[0] = tone->bend_range[0];
        n->bend_range[1] = tone->bend_range[1];
        n->pitch = note_pitch(e->number, tone->center, 64, 0);
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

/* The paired controller marks the beginning of the forest phrase on its first traversal. */
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
    case BFM_SQV_PITCH_BEND:
        o->wheel[e->channel] = e->number;
        for (unsigned i = 0; i < VOICES; ++i) {
            Note *n = &o->notes[i];
            if (!n->sample || n->channel != e->channel || n->releasing) continue;
            if (e->number != 64 && (n->bend_range[0] != 3 || n->bend_range[1] != 3)) return 0;
            if (e->number < 64 && (n->key != 65 || n->center != 64)) return 0;
            n->pitch = note_pitch(n->key, n->center, e->number, n->bend_range[0]);
        }
        return 1;
    case BFM_SQV_PROGRAM:
        o->program[e->channel] = e->number;
        return 1;
    case BFM_SQV_CONTROL:
        /* The schedule admitted these controls before rendering. */
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
        left_right[c] = limit_sample(channels[c]);
}

static int known_wheel(unsigned channel, unsigned program, unsigned wheel)
{
    if (wheel == 64) return 1;
    if (channel == 1 && program == 2) return wheel == 67;
    if (channel == 7 && program == 6)
        return wheel == 60 || wheel == 56 || wheel == 52 || wheel == 48 || wheel == 43;
    return 0;
}

/* Keep fractional event deadlines through device and output conversion. */
static int prepare_schedule(BfmForestScore *score)
{
    const BfmSqvTrack *track = &score->track;
    if (!track->event_count || track->event_count > SIZE_MAX / sizeof(*score->deadlines)) return 0;
    score->deadlines = malloc(track->event_count * sizeof(*score->deadlines));
    if (!score->deadlines) return 0;
    uint64_t elapsed = 0;
    uint64_t denominator = (uint64_t)track->ticks_per_quarter * 1000000;
    unsigned program[CHANNELS] = {0};
    uint32_t previous = 0, tempo = 500000;
    if (!denominator) return 0;
    score->divisor = denominator;
    score->field_duration = denominator * (DEVICE_RATE / 60);
    for (size_t i = 0; i < track->event_count; ++i) {
        const BfmSqvEvent *e = &track->events[i];
        uint64_t delta = e->tick - previous;
        if (e->tick < previous || !tempo || delta > (UINT64_MAX - elapsed) / tempo) return 0;
        elapsed += delta * tempo;
        if (elapsed > (UINT64_MAX - score->field_duration) / DEVICE_RATE) return 0;
        score->deadlines[i] = elapsed * DEVICE_RATE;
        previous = e->tick;
        if (first_pass_pair(track, i)) {
            if (score->phrase) return 0;
            score->phrase = i + 2;
            score->deadlines[i + 1] = score->deadlines[i];
            ++i;
            continue;
        }
        if (e->kind == BFM_SQV_END || (e->kind == BFM_SQV_CONTROL && !initial_parameter(e)) ||
            (e->kind == BFM_SQV_PITCH_BEND && !known_wheel(e->channel, program[e->channel], e->number))) {
            uint64_t frames = elapsed * score->rate / denominator;
            if (!frames || frames > SIZE_MAX / (2 * sizeof(int16_t))) return 0;
            if (e->kind == BFM_SQV_CONTROL && e->channel == 0 && e->number == 99 && e->value == 30 &&
                i + 2 == track->event_count && track->events[i + 1].kind == BFM_SQV_END &&
                track->events[i + 1].tick == e->tick && score->phrase && score->phrase < i) {
                score->repeat_period = score->deadlines[i] - score->deadlines[score->phrase - 1];
                if (score->repeat_period < score->divisor) return 0;
            }
            score->count = i;
            score->frames = (size_t)frames;
            return 1;
        }
        if (e->kind == BFM_SQV_PROGRAM) program[e->channel] = e->number;
        if (e->kind == BFM_SQV_TEMPO) {
            tempo = e->tempo_us_per_quarter;
            /* The first due tick leads the 60 Hz score update. */
            if (!e->tick) score->first_field_phase = (uint64_t)tempo * DEVICE_RATE % score->field_duration;
        }
    }
    return 0;
}

static int next_device_sample(BfmForestScore *score, int stereo[2])
{
    for (;;) {
        if (score->pending_bend && score->bend_deadline <= score->clock &&
            (score->event == score->count || score->bend_deadline < score->deadlines[score->event])) {
            if (!event(&score->orchestra, score->pending_bend)) return 0;
            score->pending_bend = NULL;
            continue;
        }
        if (score->event < score->count) {
            uint64_t due = score->deadlines[score->event];
            if (due > score->clock) break;
            const BfmSqvEvent *e = &score->track.events[score->event++];
            if (e->kind == BFM_SQV_PITCH_BEND && e->channel == 7 && e->number < 64) {
                /* Only the last wheel consumed in one score update reaches
                 * the device. Two neighboring ticks can share this write. */
                uint64_t remainder = due % score->field_duration;
                uint64_t until = (score->field_phase + score->field_duration - remainder) % score->field_duration;
                score->bend_deadline = due + until;
                score->pending_bend = e;
            } else if (!event(&score->orchestra, e)) return 0;
        } else {
            if (!score->repeat_period || score->clock < score->deadlines[score->count]) break;
            /* Carry the fractional overshoot and releasing instruments into
             * a return to the mounted phrase marker. */
            score->clock -= score->repeat_period;
            score->field_phase = (score->field_phase + score->field_duration -
                                  score->repeat_period % score->field_duration) % score->field_duration;
            score->event = score->phrase;
        }
    }
    mix_frame(&score->orchestra, stereo);
    score->clock += score->divisor;
    return 1;
}

void bfm_forest_score_rewind(BfmForestScore *score)
{
    if (!score) return;
    Orchestra *o = &score->orchestra;
    memset(o->notes, 0, sizeof(o->notes));
    memset(o->program, 0, sizeof(o->program));
    for (unsigned i = 0; i < CHANNELS; ++i) o->pan[i] = o->wheel[i] = 64;
    score->event = 0;
    score->clock = 0;
    score->field_phase = score->first_field_phase;
    score->pending_bend = NULL;
    score->remaining = score->frames;
    score->previous[0] = score->previous[1] = 0;
    score->until_output = 0;
    score->failed = 0;
}

BfmForestScore *bfm_forest_score_load(const MusashiDiscMedia *disc, unsigned rate)
{
    if (!disc || (rate != DEVICE_RATE && rate != 37800)) return NULL;
    BfmForestScore *score = calloc(1, sizeof(*score));
    if (!score) return NULL;
    score->rate = rate;
    if (bfm_sqv_member_track_load(disc, 78, 0, &score->track) != BFM_SQV_OK ||
        !prepare_schedule(score)) goto fail;
    score->orchestra.bank = bfm_palace_sound_bank_load(disc, 78);
    if (!score->orchestra.bank) goto fail;
    /* Resolve instruments and validate the mounted note/voice ownership on
     * the game thread. The callback only accesses prepared samples. */
    bfm_forest_score_rewind(score);
    while (score->clock < score->deadlines[score->count]) {
        int unused[2];
        if (!next_device_sample(score, unused)) goto fail;
    }
    score->orchestra.samples_ready = 1;
    bfm_forest_score_rewind(score);
    return score;
fail:
    bfm_forest_score_free(score);
    return NULL;
}

size_t bfm_forest_score_mix(BfmForestScore *score, int16_t *stereo, size_t frames)
{
    if (!score || !stereo || score->failed) return 0;
    if (!score->repeat_period && frames > score->remaining) frames = score->remaining;
    if (frames > SIZE_MAX / (2 * sizeof(*stereo))) return 0;
    size_t written = 0;
    while (written < frames) {
        int current[2];
        if (!next_device_sample(score, current)) {
            score->failed = 1;
            break;
        }
        if (score->until_output <= 0) {
            int fraction = -score->until_output;
            for (unsigned c = 0; c < 2; ++c) {
                int value = current[c] +
                    (int)((int64_t)(score->previous[c] - current[c]) * fraction / score->rate);
                stereo[2 * written + c] = limit_sample(stereo[2 * written + c] + value);
            }
            ++written;
            score->until_output += DEVICE_RATE;
        }
        score->until_output -= (int)score->rate;
        score->previous[0] = current[0];
        score->previous[1] = current[1];
    }
    if (!score->repeat_period) score->remaining -= written;
    return written;
}

void bfm_forest_score_free(BfmForestScore *score)
{
    if (!score) return;
    bfm_palace_sound_bank_free(score->orchestra.bank);
    for (unsigned i = 0; i < SAMPLES; ++i) bfm_spu_sample_free(&score->orchestra.samples[i]);
    bfm_sqv_track_free(&score->track);
    free(score->deadlines);
    free(score);
}
