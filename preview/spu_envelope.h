/* SPDX-License-Identifier: MIT */
#ifndef BFM_SPU_ENVELOPE_H
#define BFM_SPU_ENVELOPE_H

#include <stdint.h>

typedef enum {
    BFM_SPU_ENVELOPE_ATTACK,
    BFM_SPU_ENVELOPE_DECAY,
    BFM_SPU_ENVELOPE_SUSTAIN,
    BFM_SPU_ENVELOPE_RELEASE
} BfmSpuEnvelopeStage;

typedef struct {
    uint16_t adsr1;
    uint16_t adsr2;
    uint16_t level;
    BfmSpuEnvelopeStage stage;
    uint32_t phase;
} BfmSpuEnvelope;

/* Start a voice from silence, or begin release from its current level.
 * Supplied ADSR words are retained until the next start. */
void bfm_spu_envelope_start(BfmSpuEnvelope *env, uint16_t adsr1, uint16_t adsr2);
void bfm_spu_envelope_release(BfmSpuEnvelope *env);

/* Advance one 44,100 Hz mixer frame and return the new level, 0..32767.
 * Verified for the observed palace score and rising tone. Other register
 * settings use the same rate model and have no original playback coverage.
 * The caller owns note timing, sample termination, pan and reverb. */
uint16_t bfm_spu_envelope_clock(BfmSpuEnvelope *env);

#endif
