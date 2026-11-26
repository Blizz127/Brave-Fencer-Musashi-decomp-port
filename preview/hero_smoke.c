/* SPDX-License-Identifier: MIT */
#include "hero_smoke.h"
#include <stdint.h>

static int floor_fixed(int64_t value, unsigned fractional_bits)
{
    int64_t unit = (int64_t)1 << fractional_bits;
    return (int)(value >= 0 ? value / unit : -((-value + unit - 1) / unit));
}

static void emit_puff(const BfmHeroSmokeMaterial *material, unsigned birth, unsigned age,
                      const int camera[3][3], const int translation[3],
                      BfmHeroSmokeEmit emit, void *context)
{
    /* The first puff takes the lead position. The continuation cycles through
     * the remaining mounted positions, advancing seven places at each birth. */
    unsigned origin = birth ? 1 + (7 * birth - 1) % (BFM_HERO_SMOKE_ORIGINS - 1) : 0;
    const int16_t *base = material->origin[origin];
    int y = base[1] - 4 * (int)age;
    int view[3];
    for (unsigned i = 0; i < 3; ++i) {
        int64_t dot = (int64_t)camera[i][0] * base[0] + (int64_t)camera[i][1] * y
                    + (int64_t)camera[i][2] * base[2];
        view[i] = floor_fixed(dot, 12) + translation[i];
    }
    int z = view[2];
    if (z <= 0) return;
    int64_t reciprocal = ((int64_t)256 * 65536 + z / 2) / z;
    if (reciprocal > 131071) reciprocal = 131071;
    /* Triple the already quantized camera scale. A depth of 768 distinguishes
     * this from tripling the numerator before perspective division. */
    int scale = (int)(3 * reciprocal / 16);
    int left = 160 + floor_fixed(view[0] * reciprocal, 16)
                   + floor_fixed((int64_t)material->left * scale, 12);
    int top = 120 + floor_fixed(view[1] * reciprocal, 16)
                  + floor_fixed((int64_t)material->top * scale, 12);
    int right = left + material->width * scale / 4096;
    int bottom = top + material->height * scale / 4096;
    const int quad[4][3] = {{left, top, z}, {right, top, z},
                            {left, bottom, z}, {right, bottom, z}};
    emit(context, age * 2, quad, z / 4 + 1);
}

void bfm_hero_smoke_prepare(const BfmHeroSmokeMaterial *material, unsigned opening_field,
                            const int camera[3][3], const int translation[3],
                            BfmHeroSmokeEmit emit, void *context)
{
    /* The preceding close shot has already advanced the first puff when the
     * camera turns back to the dais. Each drawn pose lasts two opening fields. */
    if (!material || !camera || !translation || !emit ||
        opening_field < 4984 || opening_field >= 5024) return;
    unsigned tick = (opening_field - 4980) / 2;
    enum { births = BFM_HERO_SMOKE_ORIGINS + 1, visible_ticks = BFM_HERO_SMOKE_CELLS - 1 };
    /* Reuse slots over the nine drawable ages. Reverse slot order gives the
     * observed issue order when additive puffs share an ordering bucket. */
    for (unsigned slot = visible_ticks; slot > 0; --slot)
        for (unsigned birth = slot - 1; birth < births; birth += visible_ticks)
            if (birth < tick && tick - birth <= visible_ticks)
                emit_puff(material, birth, tick - birth, camera, translation, emit, context);
}
