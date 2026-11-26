/* SPDX-License-Identifier: MIT */
#include "palace_effects.h"
#include "palace_flash.h"
#include "palace_rise.h"
#include "palace_score.h"
#include "palace_sound_bank.h"
#include "palace_voice.h"
#include <stdlib.h>

typedef struct {
    unsigned field;
    int16_t *pcm;
    size_t frames, position;
    int started;
} Instrument;

enum { RISE, FLASH, AFTER_FLASH, SCORE, NOTES };
enum { SCORE_FIELD = 4630, END_OF_SCORE_PREVIEW = 5430 };
struct BfmPalaceEffects {
    Instrument notes[NOTES];
};

static int build_flash(Instrument *flash, const BfmSpuSample *sample, int after)
{
    BfmDecodedSpuSample decoded;
    if (!bfm_spu_sample_decode(sample->data, sample->bytes, &decoded))
        return 0;
    flash->frames = after ? bfm_palace_after_flash_frames(&decoded) : bfm_palace_flash_frames(&decoded);
    if (flash->frames && flash->frames <= SIZE_MAX / sizeof(*flash->pcm))
        flash->pcm = malloc(flash->frames * sizeof(*flash->pcm));
    if (flash->pcm) {
        for (size_t i = 0; i < flash->frames; ++i)
            flash->pcm[i] = after ? bfm_palace_after_flash_frame(&decoded, i) : bfm_palace_flash_frame(&decoded, i);
    }
    bfm_spu_sample_free(&decoded);
    return flash->pcm != NULL;
}

BfmPalaceEffects *bfm_palace_effects_load(const MusashiDiscMedia *disc)
{
    BfmPalaceBank *bank = bfm_palace_sound_bank_load(disc, 1);
    BfmSpuSample rising, flash, after;
    BfmPalaceEffects *effects = NULL;
    if (bfm_palace_sound_sample(bank, 5, &rising) && bfm_palace_sound_sample(bank, 3, &flash) &&
        bfm_palace_sound_sample(bank, 6, &after)) {
        effects = calloc(1, sizeof(*effects));
        if (effects) {
            Instrument *rise = &effects->notes[RISE];
            rise->field = BFM_PALACE_RISE_FIELD;
            effects->notes[FLASH].field = 4560;
            /* Both original recordings place the smaller note within two fields. */
            effects->notes[AFTER_FLASH].field = 4590;
            if (!bfm_palace_rise_render(&rising, &rise->pcm, &rise->frames) ||
                !build_flash(&effects->notes[FLASH], &flash, 0) ||
                !build_flash(&effects->notes[AFTER_FLASH], &after, 1)) {
                bfm_palace_effects_free(effects);
                effects = NULL;
            }
        }
    }
    bfm_palace_sound_bank_free(bank);
    if (effects) {
        /* The second flash starts a new score. Keep the dry stereo mix
         * through the same final field as the neutral court scene. */
        Instrument *score = &effects->notes[SCORE];
        score->field = SCORE_FIELD;
        size_t frames = (uint64_t)(END_OF_SCORE_PREVIEW - SCORE_FIELD) *
                        BFM_PALACE_VOICE_RATE * 1001 / 60000;
        if (bfm_palace_score_render(disc, BFM_PALACE_VOICE_RATE, frames, &score->pcm))
            score->frames = frames;
    }
    return effects;
}

void bfm_palace_effects_reset(BfmPalaceEffects *effects)
{
    if (!effects) return;
    for (unsigned n = 0; n < NOTES; ++n) {
        effects->notes[n].started = 0;
        effects->notes[n].position = 0;
    }
}

/* Use the same opening origin as the scene. After onset the device consumes
 * the prepared samples continuously, even while a rendered frame stalls. */
static size_t start_in_buffer(Instrument *v, uint64_t now, size_t frames)
{
    if (v->started)
        return 0;
    uint64_t due = (uint64_t)v->field * 1001 * BFM_PALACE_VOICE_RATE / 60000;
    if (due > now && due - now >= frames)
        return frames;
    v->started = 1;
    if (due >= now)
        return (size_t)(due - now);
    uint64_t late = now - due;
    v->position = late < v->frames ? (size_t)late : v->frames;
    return 0;
}

static int16_t clipped(int sample)
{
    if (sample < INT16_MIN) return INT16_MIN;
    if (sample > INT16_MAX) return INT16_MAX;
    return (int16_t)sample;
}

void bfm_palace_effects_mix(BfmPalaceEffects *effects, uint64_t opening_ms,
                            int16_t *stereo, size_t frames)
{
    if (!effects || !frames) return;
    uint64_t now = opening_ms * BFM_PALACE_VOICE_RATE / 1000;
    size_t start[NOTES], first = frames;
    for (unsigned n = 0; n < NOTES; ++n) {
        Instrument *v = &effects->notes[n];
        start[n] = start_in_buffer(v, now, frames);
        if (v->position < v->frames && start[n] < first) first = start[n];
    }
    for (size_t i = first; i < frames; ++i) {
        int mono = 0;
        for (unsigned n = RISE; n <= AFTER_FLASH; ++n) {
            Instrument *v = &effects->notes[n];
            if (i >= start[n] && v->position < v->frames)
                mono += v->pcm[v->position++];
        }
        int left = stereo[2 * i] + mono, right = stereo[2 * i + 1] + mono;
        Instrument *score = &effects->notes[SCORE];
        if (i >= start[SCORE] && score->position < score->frames) {
            left += score->pcm[2 * score->position];
            right += score->pcm[2 * score->position + 1];
            ++score->position;
        }
        stereo[2 * i] = clipped(left);
        stereo[2 * i + 1] = clipped(right);
    }
}

void bfm_palace_effects_free(BfmPalaceEffects *effects)
{
    if (effects) {
        for (unsigned n = 0; n < NOTES; ++n)
            free(effects->notes[n].pcm);
        free(effects);
    }
}
