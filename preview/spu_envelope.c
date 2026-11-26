/* SPDX-License-Identifier: MIT */
#include "spu_envelope.h"

enum { FULL_LEVEL = 32767 };

void bfm_spu_envelope_start(BfmSpuEnvelope *env, uint16_t adsr1, uint16_t adsr2)
{
    env->adsr1 = adsr1;
    env->adsr2 = adsr2;
    env->level = 0;
    env->stage = BFM_SPU_ENVELOPE_ATTACK;
    env->phase = 0;
}

void bfm_spu_envelope_release(BfmSpuEnvelope *env)
{
    if (env->stage != BFM_SPU_ENVELOPE_RELEASE) {
        env->stage = BFM_SPU_ENVELOPE_RELEASE;
        env->phase = 0;
    }
}

uint16_t bfm_spu_envelope_clock(BfmSpuEnvelope *env)
{
    const uint16_t a = env->adsr1;
    const uint16_t b = env->adsr2;
    unsigned shift;
    int delta;
    int exponential;

    switch (env->stage) {
    case BFM_SPU_ENVELOPE_ATTACK:
        shift = (a >> 10) & 31u;
        delta = 7 - ((a >> 8) & 3);
        exponential = (a & 0x8000) != 0;
        break;
    case BFM_SPU_ENVELOPE_DECAY:
        shift = (a >> 4) & 15u;
        delta = -8;
        exponential = 1;
        break;
    case BFM_SPU_ENVELOPE_SUSTAIN:
        shift = (b >> 8) & 31u;
        delta = (b & 0x4000) ? ((b >> 6) & 3) - 8 : 7 - ((b >> 6) & 3);
        exponential = (b & 0x8000) != 0;
        break;
    case BFM_SPU_ENVELOPE_RELEASE:
        shift = b & 31u;
        delta = -8;
        exponential = (b & 0x20) != 0;
        break;
    default:
        return env->level;
    }

    uint32_t period = shift > 11u ? UINT32_C(1) << (shift - 11u) : 1u;
    if (shift < 11u)
        delta *= 1 << (11u - shift);
    if (exponential && delta > 0 && env->level >= 0x6000) {
        if (shift < 10u)
            delta /= 4;
        else
            period *= 4u;
    }
    if (++env->phase < period)
        return env->level;

    env->phase = 0;
    if (exponential && delta < 0)
        delta = -((-delta * (int)env->level + FULL_LEVEL) / (FULL_LEVEL + 1));

    int level = (int)env->level + delta;
    if (level < 0)
        level = 0;
    if (level > FULL_LEVEL)
        level = FULL_LEVEL;
    env->level = (uint16_t)level;

    const int sustain_level = ((a & 15) + 1) * 2048;
    if (env->stage == BFM_SPU_ENVELOPE_ATTACK && level == FULL_LEVEL)
        env->stage = BFM_SPU_ENVELOPE_DECAY;
    if (env->stage == BFM_SPU_ENVELOPE_DECAY && level <= sustain_level)
        env->stage = BFM_SPU_ENVELOPE_SUSTAIN;
    return env->level;
}
