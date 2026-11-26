/* SPDX-License-Identifier: MIT */
#include "spu_sample.h"

#include <stdlib.h>

enum { BLOCK_BYTES = 16, BLOCK_FRAMES = 28, MAX_LOOP_PASSES = 64 };

typedef struct {
    int last, older;
} Predictor;

/* A VAB sample length is a 16-bit count of eight-byte units. The first
 * end flag terminates the waveform even when more blocks pad the slot. */
static int playback_range(const uint8_t *data, size_t bytes, size_t *end, size_t *loop)
{
    if (!data || !bytes || bytes > UINT16_MAX * 8u || bytes % BLOCK_BYTES) return 0;
    size_t start = bytes;
    for (size_t i = 0; i < bytes; i += BLOCK_BYTES) {
        unsigned control = data[i], flags = data[i + 1];
        if ((control >> 4) > 4 || (control & 15) > 12 || flags > 7) return 0;
        if (flags & 4) start = i;
        if (flags & 1) {
            if ((flags & 2) && start == bytes) return 0;
            *end = i + BLOCK_BYTES;
            *loop = (flags & 2) ? start : *end;
            return 1;
        }
    }
    return 0;
}

static void decode_blocks(const uint8_t *data, size_t bytes, Predictor *history, int16_t *out)
{
    static const int weights[5][2] = {{0, 0}, {60, 0}, {115, -52}, {98, -55}, {122, -60}};
    for (size_t i = 0; i < bytes; i += BLOCK_BYTES) {
        const uint8_t *block = data + i;
        const int *w = weights[block[0] >> 4];
        int scale = 1 << (12 - (block[0] & 15));
        for (unsigned j = 0; j < BLOCK_FRAMES; ++j) {
            int nibble = (block[2 + j / 2] >> (4 * (j % 2))) & 15;
            if (nibble >= 8) nibble -= 16;
            int prediction = (history->last * w[0] + history->older * w[1]) / 64;
            int value = nibble * scale + prediction;
            if (value > 32767) value = 32767;
            if (value < -32768) value = -32768;
            *out++ = (int16_t)value;
            history->older = history->last;
            history->last = value;
        }
    }
}

int bfm_spu_sample_decode(const uint8_t *data, size_t bytes, BfmDecodedSpuSample *out)
{
    if (!out) return 0;
    *out = (BfmDecodedSpuSample){0};
    size_t end, loop;
    if (!playback_range(data, bytes, &end, &loop)) return 0;
    size_t prefix = loop / BLOCK_BYTES * BLOCK_FRAMES;
    size_t cycle = (end - loop) / BLOCK_BYTES * BLOCK_FRAMES;
    int16_t *pcm = malloc((prefix + cycle) * sizeof(*pcm));
    if (!pcm) return 0;
    Predictor state = {0};
    decode_blocks(data, loop, &state, pcm);
    if (!cycle) {
        *out = (BfmDecodedSpuSample){pcm, prefix, prefix};
        return 1;
    }

    /* Repeating the encoded block address preserves both predictor samples.
     * A repeat in PCM is exact once a loop-entry history recurs. Bound the
     * expansion when a damaged or unsupported stream fails to settle. */
    Predictor entries[MAX_LOOP_PASSES];
    for (size_t pass = 0; pass < MAX_LOOP_PASSES; ++pass) {
        size_t cursor = prefix + pass * cycle;
        if (pass) {
            int16_t *larger = realloc(pcm, (cursor + cycle) * sizeof(*pcm));
            if (!larger) break;
            pcm = larger;
        }
        entries[pass] = state;
        decode_blocks(data + loop, end - loop, &state, pcm + cursor);
        for (size_t i = 0; i <= pass; ++i) {
            if (entries[i].last == state.last && entries[i].older == state.older) {
                *out = (BfmDecodedSpuSample){pcm, cursor + cycle, prefix + i * cycle};
                return 1;
            }
        }
    }
    free(pcm);
    return 0;
}

void bfm_spu_sample_free(BfmDecodedSpuSample *sample)
{
    if (sample) {
        free(sample->pcm);
        *sample = (BfmDecodedSpuSample){0};
    }
}
