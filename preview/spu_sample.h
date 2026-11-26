/* SPDX-License-Identifier: MIT */
#ifndef BFM_SPU_SAMPLE_H
#define BFM_SPU_SAMPLE_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    int16_t *pcm;
    size_t frames;
    size_t loop_start;
} BfmDecodedSpuSample;

/* Decode one VAB sample. Start with an unused output, or free its old PCM
 * before calling again. Failure gives an empty output. A one-shot has
 * loop_start == frames. Otherwise repeat [loop_start, frames). */
int bfm_spu_sample_decode(const uint8_t *data, size_t bytes, BfmDecodedSpuSample *out);
void bfm_spu_sample_free(BfmDecodedSpuSample *sample);

#endif
