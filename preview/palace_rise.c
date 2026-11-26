/* SPDX-License-Identifier: MIT */
#include "palace_rise.h"
#include "spu_sample.h"
#include <math.h>
#include <stdlib.h>

/* The opening advances at 60000/1001 fields per second. The original
 * releases this voice 289 fields after its first active observation. */
enum { RELEASE_FIELD = 289, MIX_RATE = 44100, VOLUME = 4050 };

static unsigned rise_pitch(unsigned field)
{
    unsigned steps = 2 * (field + 1);
    if (field > RELEASE_FIELD) steps += field - RELEASE_FIELD;
    if (steps > 763) steps = 763;
    unsigned key = steps / 64;
    unsigned start = (unsigned)(2048.0 * exp2(key / 12.0));
    unsigned end = (unsigned)(2048.0 * exp2((key + 1) / 12.0));
    return start + (end - start) * (steps % 64) / 64;
}

/* Count the four-tick updates in this instrument's exponential release. */
static unsigned release_frames(void)
{
    unsigned ticks = 0;
    for (unsigned envelope = 32767; envelope; envelope -= (envelope + 4095) / 4096)
        ticks += 4;
    return (ticks * (uint64_t)BFM_PALACE_RISE_RATE + MIX_RATE - 1) / MIX_RATE;
}

static unsigned field_frame(unsigned field)
{
    return ((uint64_t)field * BFM_PALACE_RISE_RATE * 1001 + 59999) / 60000;
}

int bfm_palace_rise_render(const BfmSpuSample *instrument, int16_t **mono, size_t *frames)
{
    if (!mono || !frames) return 0;
    *mono = NULL;
    *frames = 0;
    if (!instrument) return 0;
    BfmDecodedSpuSample sample = {0};
    if (!bfm_spu_sample_decode(instrument->data, instrument->bytes, &sample)) return 0;
    if (sample.loop_start >= sample.frames) {
        bfm_spu_sample_free(&sample);
        return 0;
    }

    const unsigned length = field_frame(RELEASE_FIELD) + release_frames();
    int16_t *buffer = malloc(length * sizeof(*buffer));
    if (!buffer) {
        bfm_spu_sample_free(&sample);
        return 0;
    }

    const uint64_t unit = 4096ULL * BFM_PALACE_RISE_RATE;
    const uint64_t end = sample.frames * unit;
    const uint64_t loop_length = (sample.frames - sample.loop_start) * unit;
    uint64_t phase = 0, envelope_clock = 0;
    unsigned envelope = 0, n = 0;
    for (unsigned field = 0; n < length; ++field) {
        unsigned field_end = field_frame(field + 1);
        if (field_end > length) field_end = length;
        if (field == RELEASE_FIELD) envelope_clock = 0;
        unsigned pitch = rise_pitch(field);
        while (n < field_end) {
            size_t i = phase / unit;
            size_t j = i + 1 == sample.frames ? sample.loop_start : i + 1;
            int64_t f = (int64_t)(phase % unit);
            int64_t value = sample.pcm[i] +
                (sample.pcm[j] - sample.pcm[i]) * f / (int64_t)unit;
            buffer[n++] = (int16_t)(value * VOLUME * envelope / (16384LL * 32767));
            phase += (uint64_t)pitch * MIX_RATE;
            if (phase >= end) phase -= loop_length;

            envelope_clock += MIX_RATE;
            if (envelope_clock >= 4u * BFM_PALACE_RISE_RATE) {
                envelope_clock -= 4u * BFM_PALACE_RISE_RATE;
                if (field < RELEASE_FIELD) {
                    envelope = envelope < 32761 ? envelope + 6 : 32767;
                } else {
                    envelope -= (envelope + 4095) / 4096;
                }
            }
        }
    }
    bfm_spu_sample_free(&sample);
    *mono = buffer;
    *frames = length;
    return 1;
}
